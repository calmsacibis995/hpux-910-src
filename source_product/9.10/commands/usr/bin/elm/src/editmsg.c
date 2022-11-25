/**			editmsg.c			**/

/*
 *  @(#) $Revision: 70.5 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  This contains routines to do with starting up and using an editor (or two)
 *  from within Elm.  This stuff used to be in mailmsg2.c...
 *
 */

#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include <termio.h>

#include "headers.h"


#ifdef NLS
# define NL_SETN   12
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


int
edit_the_message( filename, already_has_text, mode )

	char           *filename;
	int             already_has_text,
			mode;

{
	/*
	 *  Invoke the users editor on the filename.  Return when done. 
	 *  If 'already has text' then we can't use the no-editor option
	 *  and must use 'alternative editor' (e.g. $EDITOR or default_editor)
	 *  instead... 
	 *  'mode' indicates from where this module is called. If mode=1,
	 *  then called from screen oriented mode, mode=0, then called
	 *  from line oriented mode.
	 */


	int		start;
	char		*argv[12],
			buffer[10][LONG_FILE_NAME],
			editor_name[LONG_FILE_NAME];
	register int    i, j,
			stat, 
			return_value = 0;


	editor_name[0] = '\0';

	for ( i = 0; i < 10; i++ )
		buffer[i][0] = '\0';

	if ( equal(editor, "builtin") || equal(editor, "none") ) {
		if ( already_has_text )
			strcpy( editor_name, alternative_editor );
		else {
			return_value = no_editor_edit_the_message(filename);
			return( return_value );
		}
	}

	if ( mode ) {
		SetCursor( LINES, 0 );
		PutLine0( LINES, 0, catgets(nl_fd,NL_SETN,1,"invoking editor...") );
		CleartoEOLN();
		fflush( stdout );
	} else
		printf( catgets(nl_fd,NL_SETN,2,"\r\ninvoking editor...\r\n") );

	if ( strlen( editor_name ) == 0 )
			strcpy( editor_name, editor );

	if ( equal( editor_name, "vi" ) )
		strcpy( editor_name, default_editor );

	chown( filename, userid, groupid );	/* file was owned by root! */
	chmod( filename, 0600);

	/* For the user who invoke with command options ... */

	for ( start = 0, i = 0; start != -1 && i < 10; i++ )
		(void)get_word( editor_name, &start, &buffer[i][0] );

	if ( i >= 10 ) {
		dprint( 1, (debugfile, 
			"Too much options for the editor\n") );

		if ( mode ) {
			ClearLine( LINES - 1 );

			set_error( catgets(nl_fd,NL_SETN,3,"Can't invoke editor - too much options") );
		} else
			printf( catgets(nl_fd,NL_SETN,4,"Can't invoke editor - too much options\r\n") );

		(void) sleep( 2 );

		return( 1 );
	} else {
		argv[0] = get_last( &buffer[0][0] );

		for ( j = 1; j < i; j++ )
			argv[j] = &buffer[j][0];

		argv[j++] = filename;
		argv[j++] = NULL;
	}

	Raw( OFF );

	if ( has_transmit )
		transmit_functions( OFF );	/* function keys are local */

	if ( (stat = system_call(&buffer[0][0], EX_CMD, argv)) == 127 ) {
#ifdef lint
	        stat = stat;
#endif
		dprint( 1, (debugfile,
		        "System call failed with stat %d (edit_the_message)\n",
			stat) );
		dprint( 1, (debugfile, "** %s - %s **\n", error_name(errno),
			error_description(errno)) );

		if ( mode ) {
			ClearLine( LINES - 1 );
			set_error1( catgets(nl_fd,NL_SETN,5,"Can't invoke editor '%s' for composition."),
		        	editor_name );
		} else
			printf( catgets(nl_fd,NL_SETN,6,"Can't invoke editor '%s' for composition.\r\n"),
		        	editor_name );

		return_value = 1;
	}

	if ( has_transmit )
		transmit_functions( ON );	/* function keys are local */

	Raw( ON );

	return ( return_value );

}


extern char     to[VERY_LONG_STRING], 
		cc[VERY_LONG_STRING], 
		expanded_to[VERY_LONG_STRING], 
		expanded_cc[VERY_LONG_STRING], 
		subject[SLEN];

int             interrupts_while_editing;	/* keep track 'o dis stuff         */
jmp_buf         edit_location;			/* for getting back from interrupt */


#ifdef ALLOW_BCC
extern char     bcc[VERY_LONG_STRING], 
		expanded_bcc[VERY_LONG_STRING];
#endif


long            fsize();


int
no_editor_edit_the_message( filename )

	char           *filename;

{
	/*
	 *  If the current editor is set to either "builtin" or "none", then
	 *  invoke this program instead.  As it turns out, this and the 
	 *  routine above have a pretty incestuous relationship (!)...
	 */


	FILE            *edit_fd;
	char            buffer[VERY_LONG_STRING], 
			buf[SLEN],
			ch,
			editor_name[LONG_FILE_NAME]; 
	int		work;
	void            edit_interrupt();
	void		(*oldint) (), 
			(*oldquit) ();


	/*
	 *  Set up differently if we could hit a name server along the
	 *  way, to ensure we don't hit it more than one time.
	 */


#ifdef ENABLE_NAMESERVER
	strcpy( to, expanded_to );
	strcpy( cc, expanded_cc );

# ifdef ALLOW_BCC
	strcpy( bcc, expanded_bcc );
# endif

#endif


	Raw( OFF );

	if ( (edit_fd = fopen(filename, "a")) == NULL ) {
		set_error1( catgets(nl_fd,NL_SETN,7,"Couldn't open %s for appending"), filename );
		(void) sleep( 2 );

		dprint( 1, (debugfile,
		        "Error encountered trying to open file %s;\n", filename));
		dprint( 1, (debugfile, "** %s - %s **\n", error_name(errno),
			error_description(errno)) );

		Raw( ON );
		return ( 1 );
	}
	
	/*
	 *  is there already text in this file? 
	 */

	if ( fsize(edit_fd) > 0L )
		printf( catgets(nl_fd,NL_SETN,8,"\n\nPlease continue message, '.' to end, %cp to list, %c? for help;\n\n"),
		        escape_char, escape_char );
	else
		printf( catgets(nl_fd,NL_SETN,9,"\n\nPlease enter message, '.' to end, or %c? <return> for help;\n\n"),
		        escape_char );

	oldint = signal( SIGINT, edit_interrupt );
	oldquit = signal( SIGQUIT, edit_interrupt );

	interrupts_while_editing = 0;

	if ( setjmp(edit_location) != 0 ) {
		if ( interrupts_while_editing > 1 ) {
			Raw( ON );

			(void) signal( SIGINT, oldint );
			(void) signal( SIGQUIT, oldquit );

			if ( edit_fd != NULL )	/* insurance... */
				fclose( edit_fd );
			return ( 1 );
		}

		goto more_input;		/* read input again, please! */
	}

raw_input:
	Raw( ON );

more_input:
	buffer[0] = '\0';

	while ( optionally_enter(buffer, -1, -1, FALSE) == 0 ) {

		interrupts_while_editing = 0;	/* reset to zero... */

		if ( equal(buffer, ".") )
			break;			/* '.' is as good as a ^D to us */

		if ( buffer[0] == escape_char ) {
			Raw( OFF );

			ch = (char)tolower( (int)buffer[1] );
			switch ( ch ) {
			case '?':
				tilde_help();
				printf( catgets(nl_fd,NL_SETN,10,"(continue)\n\r") );
				goto raw_input;

			case TILDE:
				move_left( buffer, 1 );
				Raw( ON );
				goto tilde_input;	/* !! */

			case 't':
				get_with_expansion( "\r\nTo: ", to, expanded_to,
						    buffer );
				goto raw_input;

#ifdef ALLOW_BCC
			case 'b':
				get_with_expansion( "\r\nBcc: ", bcc, expanded_bcc,
						    buffer );
				goto raw_input;
#endif

			case 'c':
				get_with_expansion( "\r\nCc: ", cc, expanded_cc,
						    buffer );
				goto raw_input;

			case 's':
				get_with_expansion( "\r\nSubject: ", subject,
						    (char *)NULL, buffer );
				goto raw_input;

			case 'h':
				get_with_expansion( "\r\nTo: ", to, expanded_to, (char *)NULL );
				get_with_expansion( "Cc: ", cc, expanded_cc, (char *)NULL );

#ifdef ALLOW_BCC
				get_with_expansion( "Bcc: ", bcc, expanded_bcc, (char *)NULL );
#endif

				get_with_expansion( "Subject: ", subject, (char *)NULL, (char *)NULL );
				goto raw_input;

			case 'r':
				read_in_file( edit_fd, (char *) buffer + 2, 1 );
				goto raw_input;

			case 'e':
			case 'v':
				editor_name[0] = '\0';

				if ( ch == 'e' )
					strcpy( editor_name, alternative_editor );
				else
					strcpy( editor_name, visual_editor );
				
				work = 0;
				get_word( editor_name, &work, buffer );

				if ( access(buffer, ACCESS_EXISTS) == 0 ) {
					strcpy( buffer, editor );
					strcpy( editor, editor_name );
					fclose( edit_fd );
					(void) edit_the_message( filename, 0, 0 );
					strcpy( editor, buffer );
					edit_fd = fopen( filename, "a" );
					printf( catgets(nl_fd,NL_SETN,11,"(continue entering message, '.' to end)\r\n"));
					goto raw_input;
				} else
					printf( catgets(nl_fd,NL_SETN,12,"\r\n(Can't find '%s' on this system!  continue)\r\n"), 
					    editor_name );
				goto raw_input;

			case 'o':
				printf( catgets(nl_fd,NL_SETN,13,"\r\nPlease enter the name of the editor : ") );
				editor_name[0] = '\0';

				Raw( ON );
				optionally_enter( editor_name, -1, -1, FALSE );
				Raw( OFF );

				work = 0;
				get_word( editor_name, &work, buffer );

				if ( strlen(buffer) > 0 
				     && access(buffer, ACCESS_EXISTS) == 0 ) {
					strcpy( buffer, editor );
					strcpy( editor, editor_name );
					fclose( edit_fd );
					(void) edit_the_message( filename, 0, 0 );
					strcpy( editor, buffer );
					edit_fd = fopen( filename, "a" );
					printf( catgets(nl_fd,NL_SETN,11,"(continue entering message, '.' to end)\r\n"));
					goto raw_input;
				} else
					printf( catgets(nl_fd,NL_SETN,15,
					    "\r\n(Can't find '%s' on this system!  continue)\r\n"), 
					    editor_name );
				goto raw_input;

			case '<':
				(void) putchar( '\r' );
				(void) putchar( '\r' );
				if ( strlen(buffer) < 3 )
					printf( catgets(nl_fd,NL_SETN,16,"(you need to use a specific command here.  Continue...)\r\n"));
				else {
					sprintf( buf, " > %s/%s.%d 2>&1", 
						tmpdir, temp_edit, getpid() );
					strcat( buffer, buf );

					(void) system_call( (char *)buffer+2, SH, (char **)NULL );

					sprintf( buffer, "~r %s/%s.%d", tmpdir, temp_edit, getpid() );
					read_in_file( edit_fd, (char *) buffer + 3, 0 );
					(void) unlink( (char *) buffer + 3 );
				}
				goto raw_input;

			case '!':
				(void) putchar( '\n' );
				(void) putchar( '\r' );

				if ( strlen(buffer) < 3 )
					(void) system_call( shell, USER_SHELL, (char **)NULL );
				else
					(void) system_call( (char *)buffer+2, USER_SHELL, (char **)NULL );

				printf( catgets(nl_fd,NL_SETN,10,"(continue)\n\r") );
				goto raw_input;

			case 'm':	/* 
					 * same as 'f' but with leading
					 * prefix added 
					 */

			case 'f':	/* 
					 * this can be directly translated
					 * into a 'readmail' call with the
					 * same params! 
					 */

				(void) putchar( '\r' );
				(void) putchar( '\n' );
				read_in_messages( edit_fd, (char *) buffer + 1 );
				goto raw_input;

			case 'p':	/* 
					 * print out message so far.  So simple! 
					 */

				print_message_so_far( edit_fd, filename );
				goto raw_input;

			default:
				printf( catgets(nl_fd,NL_SETN,18,"\r\n(don't know what %c%c is.  Try %c? for help)\r\n"),
				    escape_char, buffer[1], escape_char );
			}

		} else {
	tilde_input:
			if ( fprintf(edit_fd, "%s\n", buffer) == EOF ){
			        int err = errno;
			        printf( catgets(nl_fd,NL_SETN,19,"(Warning: couldn't append to edit buffer - %s .....Continue.)"),
				error_description(err) );
			}	
			printf( "\r\n" );
		}

		buffer[0] = '\0';
	}

	printf( catgets(nl_fd,NL_SETN,20,"\r\n<end-of-message>\n\r\n\r") );

	Raw( ON );

	(void) signal( SIGINT, oldint );
	(void) signal( SIGQUIT, oldquit );

	if ( edit_fd != NULL )			/* insurance... */
		fclose( edit_fd );

	return ( 0 );
}


int 
tilde_help()

{
	/*
	 * a simple routine to print out what is available at this level 
	 */


	Raw( OFF );

	printf( catgets(nl_fd,NL_SETN,21,"\r\nAvailable options at this point are;\n\n") );
	printf( catgets(nl_fd,NL_SETN,22,"\t%c?\tPrint this help menu\n"), escape_char );

	if ( escape_char == TILDE )
		printf(catgets(nl_fd,NL_SETN,23,"\t~~\tAdd line prefixed by a single '~' character\n"));

	/*
	 * doesn't make sense otherwise... 
	 */


#ifdef ALLOW_BCC
	printf(catgets(nl_fd,NL_SETN,24,"\t%cb\tChange the addresses in the Blind-carbon-copy list\n"),
	       escape_char);
#endif


	printf(catgets(nl_fd,NL_SETN,25,"\t%cc\tChange the addresses in the Carbon-copy list\n"),
	       escape_char);
	printf(catgets(nl_fd,NL_SETN,26,"\t%ce\tInvoke the $EDITOR on the message, if possible\n"),
	       escape_char);
	printf(catgets(nl_fd,NL_SETN,27,"\t%cf\tadd the specified list of messages, or current\n"),
	       escape_char);
	printf(catgets(nl_fd,NL_SETN,28,"\t%ch\tchange all available headers (to,cc%s,subject)\n"),
	       escape_char,
#ifdef ALLOW_BCC
	       ",bcc");
#else
	       "");
#endif
	printf(catgets(nl_fd,NL_SETN,29,"\t%cm\tsame as '%cf', but with the current 'prefix'\n"),
	       escape_char, escape_char);
	printf(catgets(nl_fd,NL_SETN,30,"\t%co\tInvoke a user specified editor on the message\n"),
	       escape_char);
	printf(catgets(nl_fd,NL_SETN,31,"\t%cp\tprint out message as typed in so far\n"), escape_char);
	printf(catgets(nl_fd,NL_SETN,32,"\t%cr\tRead in the specified file\n"), escape_char);
	printf(catgets(nl_fd,NL_SETN,33,"\t%cs\tChange the subject of the message\n"), escape_char);
	printf(catgets(nl_fd,NL_SETN,34,"\t%ct\tChange the addresses in the To list\n"),
	       escape_char);
	printf(catgets(nl_fd,NL_SETN,35,"\t%cv\tInvoke the $VISUAL editor on the message\n\n"),
	       escape_char);
	printf(
	  catgets(nl_fd,NL_SETN,36,"\t%c!\texecute a unix command (or give a shell if no command)\n"),
	       escape_char);
	printf(
	  catgets(nl_fd,NL_SETN,37,"\t%c<\texecute a unix command adding the output to the message\n\n"),
	       escape_char);
	printf(
	  catgets(nl_fd,NL_SETN,38,"\t.  \tby itself on a line (or a control-D) ends the message\n\n"));

	Raw( ON );
}


int 
read_in_file( fd, filename, show_user_filename )

	FILE           *fd;
	char           *filename;
	int             show_user_filename;

{
	/*
	 *  Open the specified file and stream it in to the already opened 
	 *  file descriptor given to us.  When we're done output the number
	 *  of lines we added, if any... 
	 */


	FILE            *myfd;
	char            myfname[LONG_FILE_NAME], 
			buffer[VERY_LONG_STRING];
	register int    lines = 0;


	while ( whitespace(filename[lines]) )
		lines++;

	/*
	 *  expand any shell variables, '~' or other notation... 
	 */

	expand_env( myfname, (char *) filename + lines );

	if ( strlen(myfname) == 0 ) {
		printf( catgets(nl_fd,NL_SETN,39,"\r\n(no filename specified for file read!  Continue...)\r\n") );
		return;
	}

	if ( (myfd = user_fopen(myfname, "r")) == NULL ) {
		printf( catgets(nl_fd,NL_SETN,40,"\r\n(Couldn't read file '%s'!  Continue...)\r\n"),
		        myfname );
		return;
	}

	lines = 0;

	while ( fgets(buffer, VERY_LONG_STRING, myfd) != NULL ) {
		lines++;

		if ( fputs(buffer, fd) == EOF ){
		        int err = errno;
		        printf( catgets(nl_fd,NL_SETN,41,"\r\n(Sorry, couldn't add completely: %s)"), 
				error_description(err) );
			fflush( stdout );
			break;
		}	

		fflush( stdout );
	}

	user_fclose ( myfd );

	if ( show_user_filename )
		printf( catgets(nl_fd,NL_SETN,42,"\r\n(added %d line(s) from file %s.  Please continue...)\r\n"),
		        lines, myfname );
	else
		printf( catgets(nl_fd,NL_SETN,43,"\r\n(added %d line(s) to message.  Please continue...)\r\n"),
		        lines );

	return;
}


int 
print_message_so_far( edit_fd, filename )

	FILE           *edit_fd;
	char           *filename;

{
	/*
	 *  This prints out the message typed in so far.  We accomplish
	 *  this in a cheap manner - close the file, reopen it for reading,
	 *  stream it to the screen, then close the file, and reopen it
	 *  for appending.  Simple, but effective!
 	 *
	 *  A nice enhancement would be for this to -> page <- the message
	 *  if it's sufficiently long.  Too much work for now, though.
	 */


	char            buffer[VERY_LONG_STRING];


	fclose( edit_fd );

	if ( (edit_fd = fopen(filename, "r")) == NULL ) {
		printf( catgets(nl_fd,NL_SETN,44,"\r\nPanic: Can't open file for reading! \r\n") );
		emergency_exit();
	}

	printf( "\r\nTo: %s\r\n", format_long(expanded_to, 4) );
	printf( "Cc: %s\r\n", format_long(expanded_cc, 4) );


#ifdef ALLOW_BCC
	printf( "Bcc: %s\r\n", format_long(expanded_bcc, 5) );
#endif


	printf( "Subject: %s\r\n\n", subject );

	while ( fgets(buffer, VERY_LONG_STRING, edit_fd) != NULL )
		printf( "%s", buffer );

	fclose( edit_fd );

	if ( (edit_fd = fopen(filename, "a")) == NULL ) {
		printf( catgets(nl_fd,NL_SETN,45,"\r\nPanic: Can't reopen file for appending!\r\n") );
		emergency_exit();
	}

	printf( catgets(nl_fd,NL_SETN,46,"\r\n(continue entering message, please)\r\n") );

}


int 
read_in_messages( fd, buffer )

	FILE           *fd;
	char           *buffer;

{
	/*
	 *  Read the specified messages into the open file.  If the
	 *  first character of "buffer" is 'm' then prefix it, other-
	 *  wise just stream it in straight...Since we're using the
	 *  pipe to 'readmail' we can also allow the user to specify
	 *  patterns and such too...
	 */


	FILE            *myfd;
	char            local_buffer[VERY_LONG_STRING],
			tmp_buffer[VERY_LONG_STRING],
			*options,
			*sourceptr;
	register int    i,
			lines = 0, 
			add_prefix = 0, 
			mindex;
	int 		digit,
			file_specified = FALSE;
#ifdef SIGWINCH
	struct winsize	w_before, w_after;
	void		(*wstat) ();
#endif


	add_prefix = ( (char)tolower((int)buffer[0]) == 'm' );

	if ( strlen(readmail) + strlen(buffer) + strlen(infile)
				> LONG_SLEN ) {
		printf( "(too much options - Continue)\r\n" );
		return;
	}

	if ( strlen(++buffer) == 0 ) {
		if ( mail_only ) {
			printf( catgets(nl_fd,NL_SETN,47,"(you need to specify what message(s) to include)\r\n") );
			return;
		} else if ( selected )
			sprintf( buffer, "%d", compute_visible(current) );
		else
			sprintf( buffer, "%d", current );

		/*
		 *  else give us the current message number as a default 
		 */

	}

	if ( mail_only ) {
		if ( !strncmp(buffer, "-f", 2) || !in_string(buffer," -f") )
			sprintf( local_buffer, "%s -f %s %s", readmail, infile, buffer );
		else
			sprintf( local_buffer, "%s %s", readmail, buffer );
		file_specified = TRUE;

	} else { 
		if ( !strncmp(buffer, "-f", 2) || !in_string(buffer," -f") )
		        sprintf( local_buffer, "%s -f %s", readmail, infile );
		else {
		        sprintf( local_buffer, "%s %s", 
				 readmail, buffer );
			file_specified = TRUE;
		}

		sourceptr = buffer;

		while ( (options = get_token( sourceptr, " ", 0, FALSE )) 
			 	&& !file_specified ) {
			
			if ( strlen(local_buffer) + strlen(options) + 1
				> LONG_SLEN )
				break;

			digit = TRUE;
			sourceptr = NULL;

			for ( i=0; i < strlen(options); i++ )
				if ( !isdigit((int)*(options+i)) ) {
					digit = FALSE;
					break;
				}

			strcpy( tmp_buffer, local_buffer );

			if ( digit ) {
				mindex = atoi( options );

				if ( selected ) {

				  if ( mindex > selected )
					mindex = selected;
				  else if ( mindex < 1 && message_count > 0 )
					mindex = 1;

				  mindex = visible_to_index(mindex)+1; 

				} else if ( mindex > message_count )
					mindex = message_count;
				else if ( mindex < 1 && message_count > 0 )
					mindex = 1;

				sprintf( local_buffer, "%s %d", tmp_buffer,
				  header_table[mindex - 1].index_number );
			} else
				sprintf( local_buffer, "%s %s", tmp_buffer,
				  options );
		}
	} 

#ifdef SIGWINCH
	ioctl(0, TIOCGWINSZ, &w_before);
#endif
	if ( (myfd = popen(local_buffer, "r")) == NULL ) {
		printf( catgets(nl_fd,NL_SETN,48,"(can't get to 'readmail' command.  Sorry...)\r\n") );
		return;
	}
#ifdef SIGWINCH
	wstat = signal(SIGWINCH, SIG_IGN);
#endif

	dprint( 5, (debugfile, "** readmail call: \"%s\" **\n", local_buffer) );

	while ( fgets(local_buffer, VERY_LONG_STRING, myfd) != NULL ) {
		lines++;

		if ( add_prefix ){
			if ( fprintf(fd, "%s%s", prefixchars, local_buffer) == EOF ){
			        lines = -1;
				break;
			}	
		} else
			if ( fputs(local_buffer, fd) == EOF ){
			        lines = -1;
				break;
			}	
	}

	pclose( myfd );
#ifdef SIGWINCH
	ioctl(0, TIOCGWINSZ, &w_after);
	if ((w_before.ws_row != w_after.ws_row) ||
			(w_before.ws_col != w_after.ws_col))
		setsize(w_after.ws_row, w_after.ws_col);
	signal(SIGWINCH, wstat);
#endif

	if ( lines == 0 )
		printf( catgets(nl_fd,NL_SETN,49,"(Couldn't add the requested message.   Continue)\r\n"));
	else if ( lines < 0 )
	        printf( catgets(nl_fd,NL_SETN,50,"(Couldn't add the requested message completely. Continue)\r\n") );
	else
		printf( catgets(nl_fd,NL_SETN,51,"(added %d line(s) to message...  Please continue)\r\n"),
		        lines );

	return;
}


int 
get_with_expansion( prompt, buffer, expanded_buffer, sourcebuf )

	char            *prompt, 
			*buffer, 
			*expanded_buffer, 
			*sourcebuf;

{
	/*
	 *  This is used to prompt for a new value of the specified field.
	 *  If expanded_buffer == NULL then we won't bother trying to expand
	 *  this puppy out!  (sourcebuf could be an initial addition)
	 */


	char            mybuffer[VERY_LONG_STRING];


	Raw( ON );

	printf( prompt );
	fflush( stdout );					/* output! */

	strcpy( mybuffer, buffer );

	if ( sourcebuf != NULL ) {

		while ( !whitespace(*sourcebuf) && *sourcebuf != '\0' )
			sourcebuf++;

		if ( *sourcebuf != '\0' ) {

			while ( whitespace(*sourcebuf) )
				sourcebuf++;

			if ( strlen(sourcebuf) > 0 ) {
				if ( strlen(buffer) + strlen(sourcebuf) < SLEN ){
					strcat( mybuffer, " " );
					strcat( mybuffer, sourcebuf );
				}
			}
		}
	}

	optionally_enter( mybuffer, -1, -1, TRUE );	/* already data! */
	(void) putchar( '\r' );
	(void) putchar( '\n' );

	/*
	 *  if it's changed and we're supposed to expand addresses... 
	 */

	if ( !equal( buffer, mybuffer ) && expanded_buffer != NULL )
		if ( build_address( mybuffer, expanded_buffer ) )
			printf( "%s%s\n\r", prompt, expanded_buffer );

	strcpy( buffer, mybuffer );

	Raw( OFF );
	return;
}

 
void
edit_interrupt()

{
	/*
	 *  This routine is called when the user hits an interrupt key
	 *  while in the builtin editor...it increments the number of 
	 *  times an interrupt is hit and returns it.
	 */


	signal( SIGINT, edit_interrupt );
	signal( SIGQUIT, edit_interrupt );

	if ( interrupts_while_editing++ == 0 )
		printf( catgets(nl_fd,NL_SETN,52,"(Interrupt.  One more to cancel this letter...)\n\r") );

	longjmp( edit_location, 1 );			/* get back */
}
