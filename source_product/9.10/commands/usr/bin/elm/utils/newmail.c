/**				newmail.c			**/

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
 *   This monitor the mail arriving in a set of/a mailbox or folder.
 *  newmail runs in background and with "-w" option, it's more 
 *  suitable to have a window of its own to run in.
 *
 *  The usage parameters are:
 *
 *	-i <interval>  		how often to check for mail
 *				(default: 60 secs if newmail)
 *
 *	<filename>		name of a folder to monitor
 *				(can prefix with '+'/'=', or can
 *			 	default to the incoming mailbox)
 *
 *	<filename>=prefix	file to monitor, output with specified
 *				prefix when mail arrives.
 *
 *  If we're monitoring more than one mailbox the program will prefix
 *  each line output (if 'newmail') or each cluster of mail (if 'newmail -w')
 *  with the basename of the folder the mail has arrived in.  In the 
 *  interest of exhaustive functionality, you can also use the "=prefix"
 *  suffix to specify your own strings to prefix messages with.
 *
 *  The output format is either:
 *
 *  newmail:
 *	     >> New mail from <user> - <subject>
 *	     >> Priority mail from <user> - <subject>
 *
 *	     >> <folder>: from <user> - <subject>
 *	     >> <folder>: Priority from <user> - <subject>
 *
 *  newmail -w:
 *	     <user> - <subject>
 *	     Priority: <user> - <subject>
 *
 *	     <folder>: <user> - <subject>
 *	     <folder>: Priority: <user> - <subject>\fR
 *
 */


#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "defs.h"



#ifdef NLS
# include <nl_types.h>
# define NL_SETN   1
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


#define LINEFEED		(char) 10
#define BEGINNING		0	/* seek fseek(3S) */
#define DEFAULT_INTERVAL	60
#define MAX_FOLDERS		25	/* max we can keep track of */
#define NO_SUBJECT		"(No Subject Specified)"

#define metachar(c)	( c == '!' || c == '+' || c == '=' || c == '%' )


char            *getusername(),
		*strcpy();
long            bytes();

struct folder_struct {
	char            foldername[SLEN];
	char            prefix[NLEN];
	FILE            *fd;
	long            filesize;
}               folders[MAX_FOLDERS];

int             interval_time,		/* how long to sleep between checks */
#ifdef DEBUG
		debug = 0,		/* include verbose debug output?    */
#endif
		in_window = 0,		/* are we running as 'wnewmail'?    */
		total_folders = 0,	/* # of folders we're monitoring    */
		current_folder = 0;	/* struct pointer for looping       */


#ifdef NLS
nl_catd		nl_fd;			/* message catalogue file    */
char		lang[NLEN];			/* language name for NLS     */
#endif NLS


main( argc, argv )

	int             argc;
	char            *argv[];

{
	extern char     *optarg;
	extern int      optind, 
			opterr;
	char            *ptr;
	int             c, 
			i, 
			sh_pid,
			done;
	long            lastsize, 
			newsize;	/* file size for comparison..      */


	interval_time = DEFAULT_INTERVAL;
	opterr = 0;


#ifdef PFA
	pfa_dump();
#endif PFA


#ifdef NLS
	/*
	 * initalize for native language messages.... now set ready 
	 */

	strcpy( lang, getenv( "LANG" ) );
	nl_init( lang );
	nl_fd = catopen( "newmail", 0 );
#endif NLS


	while ( (c = getopt(argc, argv, "di:w")) != EOF ) {
		switch ( c ) {


#ifdef DEBUG
		case 'd':
			debug++;
			break;
#endif

		case 'i':
			interval_time = atoi( optarg );
			break;

		case 'w':
			in_window = 1;
			break;

		default:
			usage( argv );
			exit( 1 );
		}
	}

	if ( interval_time < 10 )
		fprintf( stderr, catgets(nl_fd,NL_SETN,1,"Warning: interval set to %d second%s.  I hope you know what you're doing!\n"),
			interval_time, interval_time == 1 ? "" : "s" );

	if (optind >= argc)
		add_default_folder();
 	else {
		while (optind < argc)
			add_folder(argv[optind++]);
	 	pad_prefixes();
	}

	for ( i = 0; i < total_folders; i++ )
		printf( catgets(nl_fd,NL_SETN,6,"Newmail started: folder \"%s\"\n"), 
			 	 folders[i].foldername );

	if ( optind < argc ){
		usage( argv );
		exit( 1 );
	}

	sh_pid = getppid();

	if ( ! in_window )			/* If not window go into background */
		if ( fork() ){


#ifdef NLS
			catclose( nl_fd );
#endif NLS


			exit( 0 );
		}


#ifdef PFA
	pfa_dump();
#endif PFA


#ifndef DEBUG
	if ( in_window )
		printf( catgets(nl_fd,NL_SETN,2,"Incoming mail:\n") );
#endif

		
	while ( 1 ) {

#ifdef PFA
		pfa_dump();
#endif PFA

		if ( kill( sh_pid, 0 ) == -1 ){	/* we've lost our shell! */


#ifdef NLS
			catclose( nl_fd );
#endif NLS


			exit();
		}

		if ( !isatty(1) )	/* we're not sending output to a tty any more */
			exit();


#ifdef DEBUG
		if ( debug )
			printf( "\n----\n" );
#endif


		for ( i = 0; i < total_folders; i++ ) {


#ifdef DEBUG
		if ( debug )
			printf( "[checking folder #%d: %s]\n", i, folders[i].foldername );
#endif


			if ( folders[i].fd == (FILE *) NULL ) {

				if ( (folders[i].fd = fopen(folders[i].foldername, "r")) == NULL )
					if ( errno == EACCES ) {
						fprintf( stderr, catgets(nl_fd,NL_SETN,3,"\nPermission to monitor %s denied!\n\n"),
						     folders[i].foldername );
						sleep( 5 );
						exit( 1 );
					}
			}

			if ( (newsize = bytes(folders[i].foldername)) >
			   	 folders[i].filesize ) {	/* new mail has arrived! */


#ifdef DEBUG
				if ( debug )
					printf( 
					       "\tnew mail has arrived!  old size = %ld, new size=%ld\n",
					       folders[i].filesize, newsize );
#endif


				/*
				 * skip what we've read already... 
				 */

				if ( fseek(folders[i].fd, folders[i].filesize,
					  BEGINNING) != 0 )
					perror( "fseek()" );

				folders[i].filesize = newsize;

				if ( in_window )
					putchar( (char) 007 );	/* BEEP! */
				else
					printf( "\n\r" );	/* blank lines
							 	 * surrounding message */

				read_headers( i );		/* read and display new
							  	 * mail! */

				if ( !in_window )
					printf( "\n\r" );

			} else if ( newsize != folders[i].filesize ) {/* file SHRUNK! */

				folders[i].filesize = bytes( folders[i].foldername );
				(void)fclose( folders[i].fd );
				folders[i].fd = (FILE *)NULL;

				lastsize = folders[i].filesize;
				done = 0;

				while ( !done ) {
					sleep( 0 );	/* basically gives up
							 * our CPU slice */
					newsize = bytes( folders[i].foldername );

					if ( newsize != lastsize )
						lastsize = newsize;
					else
						done++;
				}

				folders[i].filesize = newsize;
			}
		}

		sleep( interval_time );
	}

}


int
read_headers( current_folder )

	int             current_folder;

{
	/*
	 *  read the headers, output as found given current_folder,
	 *  the prefix of that folder, and whether we're in a window
	 *  or not.
	 */


	char            buffer[LONG_SLEN], 
			from_whom[SLEN], 
			subject[SLEN];
	register int    subj = 0, 
			in_header = 1, 
			count = 0, 
			priority = 0;


	while ( fgets(buffer, LONG_SLEN, folders[current_folder].fd) != NULL ) {
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

			} else if ( first_word(buffer, "Priority:") )
				priority++;

			else if ( first_word(buffer, "From:") )
				parse_arpa_from( buffer, from_whom );

			else if ( buffer[0] == LINEFEED ) {
				in_header = 0;			/* in body of message! */
				show_header( priority, from_whom, subject, current_folder );
				from_whom[0] = 0;
				subject[0] = 0;
				count++;
			}
		}
	}

	return ( count );
}


int
add_folder( name )

	char           *name;

{
	/*
	 * add the specified folder to the list of folders...ignore any
	 * problems we may having finding it (user could be monitoring a
	 * mailbox that doesn't currently exist, for example) 
	 */


	char            *cp, 
			buf[SLEN];


	if ( current_folder >= MAX_FOLDERS ) {
		fprintf( stderr, catgets(nl_fd,NL_SETN,4,"Sorry, but I can only keep track of %d folders.\n"), MAX_FOLDERS );
		exit( 1 );
	}

	/*
	 * now let's rip off the suffix "=<string>" if it's there... 
	 */

	for ( cp = name + strlen(name); cp > name + 1 && *cp != '='; cp-- )
		 /* just keep stepping backwards */ ;

	/*
	 * if *cp isn't pointing to the first character we'e got something! 
	 */

	if ( cp > name + 1 ) {

		*cp++ = '\0';		/* null terminate the filename & get prefix */

		if ( metachar(*cp) )
			cp++;

		strcpy( folders[current_folder].prefix, cp );

	} else {			/* nope, let's get the basename of the file */
		for ( cp = name + strlen(name); cp > name && *cp != '/'; cp-- )
			 		/* backing up a bit... */ ;

		if ( metachar(*cp) )
			cp++;
		if ( *cp == '/' )
			cp++;

		strcpy( folders[current_folder].prefix, cp );
	}

	/*
	 * and next let's see what kind of weird prefix chars this user might
	 * be testing us with.  We can have '+'|'='|'%' to expand or a file
	 * located in the incoming mail dir... 
	 */

	if ( metachar(name[0]) )
		expand_filename(name, folders[current_folder].foldername);
	else if ( access(name, 00) == -1 ) {

		/*
		 * let's try it in the mail home directory 
		 */

		sprintf( buf, "%s%s", mailhome, name );

		if ( access(buf, 00) != -1 )	
			strcpy( folders[current_folder].foldername, buf );
		else
			strcpy( folders[current_folder].foldername, name );
	} else
		strcpy( folders[current_folder].foldername, name );

	/*
	 * now let's try to actually open the file descriptor and grab a
	 * size... 
	 */

	if ( (folders[current_folder].fd =
	     fopen(folders[current_folder].foldername, "r")) == NULL )

		if ( errno == EACCES ) {
			fprintf( stderr, catgets(nl_fd,NL_SETN,5,"\nPermission to monitor \"%s\" denied!\n\n"),
				folders[current_folder].foldername );
			exit( 1 );

		}

	folders[current_folder].filesize =
		bytes( folders[current_folder].foldername );

	/*
	 * and finally let's output what we did 
	 */


#ifdef DEBUG
	if ( debug )
		printf( "folder %d: \"%s\" <%s> %s, size = %ld\n",
		        current_folder,
		        folders[current_folder].foldername,
		        folders[current_folder].prefix,
			folders[current_folder].fd == NULL ? "not found" : "opened",
		        folders[current_folder].filesize );
#endif


	/*
	 * and increment current-folder please! 
	 */

	current_folder++;
	total_folders++;
}


int
add_default_folder()

{
	/*
	 * this routine will add the users home mailbox as the folder to
	 * monitor.  Since there'll only be one folder we'll never prefix it
	 * either... 
	 */

	sprintf( folders[0].foldername, "%s%s", mailhome, getusername() );

	folders[0].fd = fopen( folders[0].foldername, "r" );
	folders[0].filesize = bytes( folders[0].foldername );


#ifdef DEBUG
	if ( debug )
		printf( "default folder: \"%s\" <%s> %s, size = %ld\n",
		       folders[0].foldername,
		       folders[0].prefix,
		       folders[0].fd == NULL ? "not found" : "opened",
		       folders[0].filesize );
#endif


	total_folders = 1;
}


int
real_from( buffer, who )

	char            *buffer,
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
	       who, machine ); 		/* has TZ		*/

	if ( machine[0] == '\0' )
		sscanf( buffer, "%*s %s %*s %*s %*s %*s %*s %*s %*s %s",
		       who, machine );

	if ( machine[0] == '\0' )	/* try for srm address */
		sscanf( buffer, "%*s %s %*s %*s %*s %*s %*s %*s %s",
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


	chars--;		/* index starting at zero! */

	for ( i = chars; string[i] != '\0' && string[i] != '\n'; i++ )
		string[i - chars] = string[i];

	string[i - chars] = '\0';
}


int
show_header( priority, from, subject, current_folder )

	int             priority;
	char            *from, 
			*subject;
	int             current_folder;

{
	/*
	 *  output header in clean format, including abbreviation
	 *  of return address if more than one machine name is
	 *  contained within it! 
	 */


	char            buffer[SLEN];
	int             loc, 
			i = 0, 
			exc = 0;


#ifdef PREFER_UUCP

	char		BOGUS_INTERNET[SLEN],
			hostname[SLEN];

	gethostname( hostname, sizeof(hostname) );

	strcpy( BOGUS_INTERNET, "@" );
	strcat( BOGUS_INTERNET, hostname );

	if ( chloc(from, '!') != -1 && in_string(from, BOGUS_INTERNET) )
		from[strlen(from) - strlen(BOGUS_INTERNET)] = '\0';

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
		strcpy( from, buffer );
	}

	if ( strlen(subject) < 2 )
		strcpy( subject, NO_SUBJECT );

	if ( in_window )
		if ( total_folders > 1 )
			printf( "%s: %s mail from %s - %s\n",
			        folders[current_folder].prefix,
			        priority ? "Priority" : "New", from, subject );
		else
			printf( "%s mail from %s - %s\n",
			        priority ? "Priority" : "New", from, subject );

	else if ( total_folders > 1 )
		printf( ">> %s: %s mail from %s - %s\n\r",
		        folders[current_folder].prefix,
		        priority ? "Priority" : "New", from, subject );

	else
		printf( ">> %s mail from %s - %s\n\r",
		        priority ? "Priority" : "New", from, subject );
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

	no_ret( buffer );		/* blow away '\n' char! */

	if ( lastch(buffer) == '>' ) {
		for ( i = strlen("From: "); buffer[i] != '\0' && buffer[i] != '<' &&
		     buffer[i] != '('; i++ )
			temp[j++] = buffer[i];
		temp[j] = '\0';

	} else if ( lastch(buffer) == ')' ) {
		for ( i = strlen(buffer) - 2; buffer[i] != '\0' && buffer[i] != '(' &&
		     buffer[i] != '<'; i-- )
			temp[j++] = buffer[i];
		temp[j] = '\0';
		reverse( temp );
	}

	if ( strlen(temp) > 0 ) {	/* mess with buffer... */

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


long
bytes( name )

	char           *name;

{
	/*
	 *  return the number of bytes in the specified file.  This
	 *  is to check to see if new mail has arrived....  
	 */


	int             ok = 1;
	extern int      errno;	 	/* system error number! */
	struct stat     buffer;


	if ( stat(name, &buffer) != 0 )
		if ( errno != 2 ) {
			fprintf( stderr, catgets(nl_fd,NL_SETN,7,"Error %d attempting fstat on %s"), errno, name );
			exit( 1 );
		} else
			ok = 0;

	return ( ok ? buffer.st_size : 0 );
}


char *
getusername()

{
	/*
	 *  Getting the username on some systems is a real pain, so...
	 * This routine is guaranteed to return a usable username 
	 */


	char            *return_value, 
			*cuserid(), 
			*getlogin();


	if ( (return_value = (char *) getenv("LOGNAME")) == NULL )
	        if ( (return_value = getlogin()) == NULL )
	        	if ( (return_value = cuserid(NULL)) == NULL ) {
			        printf( catgets(nl_fd,NL_SETN,8,"Newmail: I can't get username!\n") );
			        exit( 1 );
		        }

	return ( (char *) return_value );
}


int
usage( argv )

	char	**argv;

{
	/*
	 * print a nice friendly usage message 
	 */

	fprintf( stderr, catgets(nl_fd,NL_SETN,9,"\nUsage: %s [-i interval] [-w] \n"), *argv );
	fprintf( stderr, catgets(nl_fd,NL_SETN,10,"\nWhere:\n") );
	fprintf( stderr, catgets(nl_fd,NL_SETN,11,"  -i D\tsets the interval checking time to 'D' seconds\n") );
	fprintf( stderr, catgets(nl_fd,NL_SETN,12,"  -w\tforces 'window'-style output\n\n") );
}


int
expand_filename( name, store_space )

	char            *name, 
			*store_space;

{
	strcpy( store_space, name );

	if ( expand(store_space) == 0 ) {
		fprintf( stderr, catgets(nl_fd,NL_SETN,13,"Sorry, but I couldn't expand \"%s\"\n"), name );
		exit( 1 );
	}
}


int
pad_prefixes()

{
	/*
	 *  This simple routine is to ensure that we have a nice
	 *  output format.  What it does is whip through the different
	 *  prefix strings we've been given, figures out the maximum
	 *  length, then space pads the other prefixes to match.
	 */


	register int    i, 
			j, 
			len = 0;


	for ( i = 0; i < total_folders; i++ )
		if ( len < (j = strlen(folders[i].prefix)) )
			len = j;

	for ( i = 0; i < total_folders; i++ )
		for ( j = strlen(folders[i].prefix); j < len; j++ )
			strcat( folders[i].prefix, " " );
}
