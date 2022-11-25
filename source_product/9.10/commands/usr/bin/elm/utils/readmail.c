/**			readmail.c			**/

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
 *  This routine adds the functionality of the "~r" command to the elm 
 *  while still allowing the user to use the editor of their choice.
 *
 *  The program, without any arguments, tries to read a file in the users home 
 *  directory called ".readmail" (actually defined in the sysdefs.h system 
 *  defines file) and if it finds it reads the current message.  If it doesn't 
 *  find it, it will return a usage error.
 *
 *  The program can also be called with an explicit message number, list of 
 *  message numbers, or a string to match in the message (including the header).
 *  NOTE that when you use the string matching option it will match the first 
 *  message containing that EXACT (case sensitive) string and then exit.
 *
 */


#include <stdio.h>
#include <ctype.h>

#include "defs.h"


#ifdef NLS
# include <nl_types.h>
# define NL_SETN  1 
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


/*
 *  three defines for what level of headers to display 
 */

#define ALL		1
#define WEED		2
#define NONE		3

#define metachar(c)	( c == '!' || c == '=' || c == '+' || c == '%' )


#define  MAX_LIST	1024	/* largest single list of arguments */

#define  LAST_MESSAGE	9999	/* last message in list ('$' char)  */
#define  LAST_CHAR	'$'	/* char to delimit last message..   */
#define  STAR		'*'	/* char to delimit all messages...  */

#define DONE	0		/* for use with the getopt	    */
#define ERROR   -1		/* library call...		    */

int             read_message[MAX_LIST];	/* list of messages to read */
int             messages = 0,	/* index into list of messages      */
		max_list = 25,	/* default max list		    */
		weedcount = 0,
		prev_weed = 0;	/* flag: using multi lines   */

int             numcmp();	/* strcmp, but for numbers          */

char		*weedlist[MAX_IN_WEEDLIST];

extern char    *optional_arg;	/* for parsing the ... 		    */
extern int      opt_index;	/* .. starting arguments           */

void		*pmalloc();

#ifdef NLS
nl_catd		nl_fd;			/* message catalogue file    */
char		lang[NLEN];			/* language name for NLS     */
#endif NLS


main( argc, argv )

	int             argc;
	char           *argv[];

{
	FILE           *file;		/* generic file descriptor! */
	char            filename[SLEN],	/* filename buffer          */
	                infile[SLEN],	/* input filename	    */
	                buffer[SLEN],	/* file reading buffer      */
	                string[SLEN];	/* string match buffer      */

	int             current_in_queue = 0,	/* these are used for...     */
	                current = 0,		/* ...going through msgs     */
	                list_all_messages = 0,	/* just list 'em all??       */
	                num,			/* for argument parsing      */
	                page_breaks = 0,	/* use "^L" breaks??         */
	                total,			/* number of msgs current    */
			use_elm_weedlist = 0,	/* flag: called from elm     */
	                include_headers = WEED,	/* flag: include msg header? */
	                last_message = 0,	/* flag: read last message?  */
	                not_in_header = 0,	/* flag: in msg header?      */
	                string_match = 0,	/* flag: using string match? */
	                displayed = 0,		/* flag: using string match? */
			messages_listed = 0;    /* flag: using paginate      */

#ifdef NLS
	/*
	 * initalize for native language messages.... now set ready 
	 */

	(void) strcpy( lang, getenv( "LANG" ) );
	(void) nl_init( lang );
	nl_fd = catopen( "readmail", 0 );
#endif NLS

	while ( (num = get_options(argc, argv, "m:nhf:pw")) > 0 ) {
		switch ( num ) {
		case 'm': {
			int nn = sscanf( optional_arg, "%d", &max_list );
			if ( (max_list < 25) || (nn < 1) )
				max_list = 25;
			if ( max_list > MAX_LIST )
				max_list = MAX_LIST;
			break;
		}
		case 'n':
			include_headers = NONE;
			break;

		case 'h':
			include_headers = ALL;
			break;

		case 'f':
			(void) strcpy( infile, optional_arg );

			if ( metachar(infile[0]) )
				if ( expand(infile) == 0 )
					printf( catgets(nl_fd,NL_SETN,1,"%s: couldn't expand filename %s!\n"),
					       argv[0], infile );
			break;

		case 'p':
			page_breaks++;
			break;
		case 'w':
			use_elm_weedlist++;
			break;
		}
	}

	if ( num == ERROR ) {
		printf( catgets(nl_fd,NL_SETN,2,"Usage: %s [-n|-h] [-f filename] [-p] <message list>\n"),
		       argv[0] );
		exit( 1 );
	}

	/*
	 *  whip past the starting arguments so that we're pointing
	 *  to the right stuff... 
	 */

	if ( argc > 1 )
		*argv++;			/* past the program name... */

	while ( opt_index-- > 1 ) {
		*argv++;
		argc--;
	}

	/*
	 *  now let's figure out the parameters to the program... 
	 */

	if ( argc == 1 ) {		/* no arguments... called from 'Elm'? */
		(void) sprintf( filename, "%s/%s", getenv("HOME"), readmail_file );

		if ( (file = fopen(filename, "r")) != NULL ) {
			fscanf( file, "%d%s", 
				&(read_message[messages++]), infile );
			(void) fclose( file );
		} else {	/* no arguments AND no .readmsg file!! */
			(void) fprintf( stderr, catgets(nl_fd,NL_SETN,2,"Usage: %s [-n|-h] [-f filename] [-p] <message list>\n"),
				        argv[0] );
			exit( 1 );
		}

	} else if ( !isdigit(*argv[0]) && *argv[0] != LAST_CHAR &&
		   *argv[0] != STAR ) {
		string_match++;
		while ( *argv )
		    (void) sprintf( string, "%s%s%s", string, string[0] == '\0' ? "" : " ",
				    *argv++ );

	} else if ( *argv[0] == STAR )		/* all messages....   */
		list_all_messages++;
	else {					/* list of nums   */

		while ( --argc > 0  &&  messages < max_list  &&  messages < MAX_LIST ) {
			num = -1;

			(void) sscanf( *argv, "%d", &num );

			if ( num < 0 ) {
				if ( *argv[0] == LAST_CHAR ) {
					last_message++;
					num = LAST_MESSAGE;
				} else {
					(void) fprintf( stderr, catgets(nl_fd,NL_SETN,4,"I don't understand what '%s' means...\n"),
						        *argv );
					exit( 1 );
				}

			} else if ( num == 0 ) {	/* another way to say "last" */
				last_message++;
				num = LAST_MESSAGE;
			}

			*argv++;

			read_message[messages++] = num;
		}

		/*
		 *  and now sort 'em to ensure they're in a reasonable order... 
		 */
		if ( argc > 0 )
			(void) fprintf( stderr,
				        catgets(nl_fd,NL_SETN,8,"Too many messages, only %d messages are read out\n" ),
				        max_list );
		qsort( (void *)read_message, (size_t)messages, sizeof(int), numcmp );
	}

	/*
	 *  Now let's get to the mail file... 
	 */
	
	if ( use_elm_weedlist )
		if ( get_weedlist() == 0 ) {
			use_elm_weedlist =0;
			include_headers = ALL;
		}
		
	if ( strlen(infile) == 0 ) {
		(void) strcpy( filename, getenv("LOGNAME") );

		if ( strlen(filename) == 0 )
	        	(void) strcpy( filename, getlogin() );

		if ( strlen(filename) == 0 )
	        	(void) strcpy( filename, cuserid((char *)NULL) );

		(void) sprintf( infile, "%s/%s", mailhome, filename );
	}

	if ( (file = fopen(infile, "r")) == NULL ) {
		(void) printf( catgets(nl_fd,NL_SETN,5,"But you have no mail! [ file = %s ]\n"),
			       infile );
		exit( 0 );
	}

	/*
	 *  Now it's open, let's display some 'ole messages!! 
	 */

	if ( string_match || last_message ) {	/* pass through it once */

		if ( last_message ) {
			total = count_messages( file );	/* instantiate count */
			for ( num = 0; num < messages; num++ )
				if ( read_message[num] == LAST_MESSAGE )
					read_message[num] = total;
		} else if ( string_match )
			match_string( file, string );	/* stick msg# in list */

		if ( total == 0 && !string_match ) {
			printf( catgets(nl_fd,NL_SETN,6,"There aren't any messages to read!\n") );
			exit( 0 );
		}
	}

	/*
	 *  now let's have some fun! 
	 */

	while ( fgets(buffer, SLEN, file) != NULL ) {
		if ( real_from(buffer) ) {
			if ( !list_all_messages ) {
				if ( current == read_message[current_in_queue] )
					current_in_queue++;
				if ( current_in_queue >= messages )
					exit( 0 );
			}
			current++;
			if ( (messages_listed &&
			     current == read_message[current_in_queue]) ||
			     (list_all_messages && current > 1) )
				if ( page_breaks )
					(void) putchar( FORMFEED );
				else {
					printf("\n");
					for (num=0; num<72; num++)
						(void) putchar('-');
					printf("\n\n\n");	
				}
			not_in_header = 0;	/* we're in the header! */
		}

		if ( current == read_message[current_in_queue] || list_all_messages )
			if ( include_headers == ALL || not_in_header ) {
				printf( "%s", buffer );
				messages_listed++;
			} else if ( strlen(buffer) < 2 ) {
				printf("\n");
				not_in_header++;
			} else if ( include_headers == WEED ) {
				if ( use_elm_weedlist ) {
					prev_weed = matches_weedlist( buffer );
					if ( !prev_weed )
						printf("%s", buffer );
				} else {
					if ( first_word(buffer, "From:") ) {
						printf( "%s", buffer );
						displayed = 1;

					} else if ( first_word(buffer, "Date:") ) {
						printf( "%s", buffer );
						displayed = 1;

					} else if ( first_word(buffer, "Subject:") ) {
						printf( "%s", buffer );
						displayed = 1;

					} else if ( first_word(buffer, "To:") ) {
						printf( "%s", buffer );
						displayed = 1;

					} else if ( displayed  && (
					  first_word(buffer, "        ") ||
					  first_word(buffer, "\t") )        )
						printf( "%s", buffer );
					else
						displayed = 0;
				}
			}
	}



#ifdef NLS
	(void) catclose( nl_fd );
#endif NLS

	exit( 0 );
#ifdef lint
	return( 0 );
#endif
}


int
count_messages( file )

	FILE           *file;

{
	/*
	 *  Returns the number of messages in the file 
	 */


	char            buffer[SLEN];
	int             count = 0;


	while ( fgets(buffer, SLEN, file) != NULL )
		if ( real_from(buffer) )
			count++;

	rewind( file );
	return ( count );
}


int
match_string( mailfile, string )

	FILE           *mailfile;
	char           *string;

{
	/*
	 *  Increment "messages" and put the number of the message
	 *  in the message_count[] buffer until we match the specified 
	 *  string... 
	 */


	char            buffer[SLEN];
	int             message_count = 0,
			prev_count = -1;


	while ( fgets(buffer, SLEN, mailfile) != NULL ) {
		if ( real_from(buffer) )
			message_count++;

		if ( in_string(buffer, string) && 
		     (message_count != prev_count) ) {
			read_message[messages++] = message_count;
			prev_count = message_count;
		}

	}

	rewind( mailfile );

	if ( messages )
		return;
	else {
		(void) fprintf( stderr,
			        catgets(nl_fd,NL_SETN,7,"Couldn't find message containing '%s'\n"),
			        string );
		exit( 1 );
	}
}


int
numcmp( a, b )

	int             *a, 
			*b;

{
	/*
	 *  compare 'a' to 'b' returning less than, equal, or greater
	 *  than, accordingly.
	 */

	return ( *a - *b );
}


int
real_from( buffer )

	char            *buffer;

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

	(void) sscanf( buffer, "%*s %*s %*s %*s %*s %s %s %s %s",
		       buff6th, buff7th, buff8th ,buff9th );

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



#define NOTWEEDOUT	0
#define WEEDOUT		1

int
get_weedlist()

{
	/*
	 *  this routine reads weedlist if weed is on
	 */


	FILE            *file;
	char            buffer[VERY_LONG_STRING], 
			filename[LONG_FILE_NAME],
	                word1[SLEN], 
			word2[LONG_FILE_NAME];
	register int    weedflg = 0,
			last = NOTWEEDOUT;

	(void) sprintf( filename, "%s/%s", getenv( "HOME" ), ".elm/elmrc" );

	if ( (file = fopen(filename, "r")) == NULL ) {
		weedflg = 1;		/* Use default weed list */
		default_weedlist();
		return( weedflg );

	}

	while ( fgets(buffer, VERY_LONG_STRING, file) != NULL ) {
		no_ret( buffer );			/* remove return */
		if ( buffer[0] == '#' )
			last = NOTWEEDOUT;	/* comment line           */
		if ( strlen(buffer) < 2 )
			last = NOTWEEDOUT;	/* empty line             */

		if ( first_word(buffer, "weed") && !first_word(buffer, "weedout") ) {
			breakup( buffer, word1, word2 );
			weedflg = is_it_on( word2 );
			last = NOTWEEDOUT;
		} else if ( first_word(buffer, "weedout") ) {
			breakup( buffer, word1, word2 );
			weedcount = 0;
			weedout( word2 );
			last = WEEDOUT;
		} else if ( last == WEEDOUT && (chloc(buffer, '=') == -1) )
			weedout( buffer );

	}

	(void) fclose( file );

	return ( weedflg );
}


int
breakup( buffer, word1, word2 )

	char            *buffer, 
			*word1, 
			*word2;

{
	/*
	 *  This routine breaks buffer down into word1, word2 where 
	 *  word1 is alpha characters only, and there is an equal
	 *  sign delimiting the two...
	 *	alpha = beta
	 *  For lines with more than one 'rhs', word2 is set to the
	 *  entire string...
	 */


	register int    i;


	for ( i = 0; buffer[i] != '\0' && ok_char(buffer[i]); i++ )
		if ( buffer[i] == '_' )
			word1[i] = '-';
		else if ( isupper(buffer[i]) )
			word1[i] = (char)tolower( (int)buffer[i] );
		else
			word1[i] = buffer[i];

	word1[i++] = '\0';			/* that's the first word! */

	/*
	 *  spaces before equal sign? 
	 */

	while ( whitespace(buffer[i]) )
		i++;

	if ( buffer[i] == '=' )
		i++;

	/*
	 *  spaces after equal sign? 
	 */

	while ( whitespace(buffer[i]) )
		i++;

	if ( buffer[i] != '\0' )
		(void) strcpy( word2, (char *) (buffer + i) );
	else
		word2[0] = '\0';

	while ( whitespace(word2[strlen(word2) - 1]) )
		word2[strlen(word2) - 1] = '\0';

	if ( word2[0] == '"' && word2[strlen(word2) - 1] == '"' ) {
		word2[strlen(word2) - 1] = '\0';
		(void) strcpy( word2, (char *) (word2 + 1) );
	}
}


int
default_weedlist()

{
	/*
	 *  Install the default headers to weed out!  Many gracious 
	 *  thanks to John Lebovitz for this dynamic method of 
	 *  allocation!
	 */

	static char    *default_list[] = { ">From", "In-Reply-To:",
				  "References:", "Newsgroups:", "Received:",
			   	  "Apparently-To:", "Message-Id:", "Content-Type:",
				  "From", "Mailer:", NULL
				};


	for ( weedcount = 0; default_list[weedcount] != (char *) 0; weedcount++ ) {

		if ( (weedlist[weedcount] =
		     pmalloc((int)strlen(default_list[weedcount]) + 1)) == NULL ) {
			(void) fprintf( stderr,
				        catgets(nl_fd,NL_SETN,11,"\n\rNot enough memory for default weedlist.  Leaving.\n\r") );
			exit( 1 );
		}

		(void) strcpy( weedlist[weedcount], default_list[weedcount] );
	}
}


int
weedout( string )

	char           *string;

{
	/*
	 *  This routine is called with a list of headers to weed out.   
	 */


	char            *strptr, 
			*header;
	register int    i,
			spaces;


	strptr = string;

	while ( (header = strtok(strptr, "\t,\"'")) != NULL) {
		if (strlen(header) > 0 ) {
			if ( weedcount > MAX_IN_WEEDLIST ) {
				(void) fprintf( stderr, catgets(nl_fd,NL_SETN,12,"Too many weed headers!  Leaving...\n") );
				exit( 1 );
			}

			if ( (weedlist[weedcount] = pmalloc((int)strlen(header) + 1))
			      				== NULL ) {
				(void) fprintf( stderr, catgets(nl_fd,NL_SETN,13,"Too many weed headers - out of memory!  Leaving...\n") );
				exit( 1 );
			}

			for ( i = 0; i < strlen(header); i++ )
				if ( header[i] == '_' )
					header[i] = ' ';
			
			spaces = TRUE;
			for ( i = 0; i < strlen(header); i++ )
				if ( header[i] != ' ' )
					spaces = FALSE;

			if ( !spaces ) {
				(void) strcpy( weedlist[weedcount], header );
				weedcount++;
			}
		}

		strptr = NULL;
	}
}


int
matches_weedlist( buffer )

	char           *buffer;

{
	/*
	 *  returns true iff the first 'n' characters of 'buffer' 
	 *  match an entry of the weedlist 
	 */


	register int    i;


	for ( i = 0; i < weedcount; i++ ) {
		if ( prev_weed == 1                  &&
		     (first_word(buffer, "        ") ||
		      first_word(buffer, "\t"      )   ) )
			return( 1 );

		if ( first_word(buffer, weedlist[i]) )
			return ( 1 );
	}
	return ( 0 );
}


int
is_it_on( word )

	char           *word;

{
	/*
	 *  Returns TRUE if the specified word is either 'ON', 'YES'
	 *  or 'TRUE', and FALSE otherwise.   We explicitly translate
	 *  to lowercase here to ensure that we have the fastest
	 *  routine possible - we really DON'T want to have this take
	 *  a long time or our startup will be major pain each time.
	 */


	static char     mybuffer[NLEN];
	register int    i, 
			j;


	for ( i = 0, j = 0; word[i] != '\0'; i++ )
		mybuffer[j++] = isupper(word[i]) 
				? (char)tolower((int)word[i]) : word[i];

	mybuffer[j] = '\0';

	return ( first_word(mybuffer, "on")  ||
		 first_word(mybuffer, "yes") ||
		 first_word(mybuffer, "true")  );
}


void *
pmalloc( size )

	int             size;

{
	/*
	 *  return the address of a specified block 
	 */


	static char    *our_block = NULL;
	static int      free_mem = 0;

	char            *return_value;

	/*
	 *  if bigger than our threshold, just do the real thing! 
	 */

	if ( size > PMALLOC_THRESHOLD )
		return ( malloc((unsigned) size) );

	/*
	 *  if bigger than available space, get more, tossing what's left 
	 */

	if ( size > free_mem ) {
		if ( (our_block = malloc((unsigned) PMALLOC_BUFFER_SIZE)) == NULL ) {
			(void) fprintf( stderr, 
				       catgets(nl_fd,NL_SETN,14,"\n\r\n\rCouldn't malloc %d bytes!!\n\r\n\r"),
				       PMALLOC_BUFFER_SIZE );
			exit( 1 );
		}

		our_block += 4;			/* just for safety, don't give back true
				 		 * address */
		free_mem = PMALLOC_BUFFER_SIZE - 4;
	}

	return_value = our_block;		/* get the memory */
	size = ( (size + 3) / 4 ) * 4;		/* Go to quad byte boundary */
	our_block += size;			/* use it up      */
	free_mem -= size;			/* and decrement */

	return ( (char *) return_value );
}


