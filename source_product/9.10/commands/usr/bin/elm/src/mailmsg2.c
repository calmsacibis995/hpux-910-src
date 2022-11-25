/** 			mailmsg2.c			**/

/*
 *  @(#) $Revision: 72.6 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  Interface to allow mail to be sent to users.  Part of ELM  
 */


#include <signal.h>

#include "headers.h"


#ifdef NLS
# define NL_SETN  26
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


extern int      no_to_header;

static FILE	*filedesc; 		/* used in write_header_info() */

FILE           *write_header_info();

/*
 * these are all defined in the mailmsg1.c file! 
 */


extern char     subject[SLEN], 
		in_reply_to[SLEN], 
		expires[SLEN], 
		action[SLEN], 
		priority[SLEN], 
		reply_to[VERY_LONG_STRING], 
		to[VERY_LONG_STRING], 
		cc[VERY_LONG_STRING], 
		expanded_to[VERY_LONG_STRING], 
		expanded_cc[VERY_LONG_STRING], 
		expanded_reply_to[VERY_LONG_STRING], 
		user_defined_header[SLEN];


#ifdef ALLOW_BCC
char            bcc[VERY_LONG_STRING], 
		expanded_bcc[VERY_LONG_STRING];
#endif


#ifdef ENCRYPTION_SUPPORTED
int             gotten_key = 0;
#endif


#ifdef BOUNCEBACK_ENABLED
char           *bounce_off_remote();
#endif


#ifdef FORMS_MODE_SUPPORTED
	int		softkeys_stat;			/* status of softkeys in FORM */
#endif


int
mail( copy_msg, edit_message, batch, form )

	int             copy_msg, 
			edit_message, 
			batch, 
			form;

{
	/*
	 *  Given the addresses and various other miscellany (specifically, 
	 *  'copy-msg' indicates whether a copy of the current message should 
	 *  be included, 'edit_message' indicates whether the message should 
	 *  be edited and 'batch' indicates that the message should be read 
	 *  from stdin) this routine will invoke an editor for the user and 
	 *  then actually mail off the message. 'form' can be YES, NO, or
	 *  MAYBE.  YES=add "Content-Type: mailform" header, MAYBE=add the
	 *  M)ake form option to last question, and NO=don't worry about it!
	 *  Also, if 'copy_msg' = FORM, then grab the form temp file and use
	 *  that...
	 *  Return value 0 means no needs of redraw screen and 1 means
	 *  it needs redraw screen.
	 */


	FILE            *reply, 
			*real_reply;			/* post-input buffer */
	char            filename[LONG_FILE_NAME], 
			filename2[LONG_FILE_NAME];


#ifdef FORMS_MODE_SUPPORTED
	char            fname[SLEN];
#endif


	char            very_long_buffer[VERY_LONG_STRING];
	char            ch;
	register int    retransmit = FALSE;
	int             already_has_text = FALSE;	/* we need an ADDRESS */
	int             err;


	static int      cancelled_msg = 0;

	dprint( 4, (debugfile, "\nMailing to \"%s\" (with%s editing)\n",
		    expanded_to, edit_message ? "" : "out") );

	/*
	 *  first generate the temporary filename 
	 */

	sprintf( filename, "%s/%s%d", tmpdir, temp_file, getpid() );
	if (access(filename, ACCESS_EXISTS) != 0)
		close(creat(filename, 0600));

	/*
	 *  if possible, let's try to recall the last message? 
	 */

	if ( !batch && copy_msg != FORM && user_level != 0 ) {
		retransmit = recall_last_msg( filename, copy_msg, &cancelled_msg,
					      &already_has_text );
		if ( retransmit == eof_char )
			return ( 0 );
	}

	/*
	 *  if we're not retransmitting, create the file.. 
	 */

	if ( !retransmit )
		if ( (reply = fopen(filename, "w")) == NULL ) {
			dprint( 1, (debugfile,
				   "Attempt to write to temp file %s failed with error %s (mail)\n",
				   filename, error_name(errno)) );

			if ( mail_only )
				printf(catgets(nl_fd,NL_SETN,1,"Could not create file %s (%s)\r\n"),
					filename, error_name(errno) );
			else
				error2(catgets(nl_fd,NL_SETN,2,"Could not create file %s (%s)"), filename,
			       		error_name(errno) );

			return ( 0 );
		}

	if ( batch ) {
		Raw( OFF );

		if ( isatty(fileno(stdin)) ) {
			fclose( reply );		/* let edit-the-message open it! */
			printf( "To: %s\nSubject: %s\n", expanded_to, subject );

			if ( edit_the_message(filename,FALSE,FALSE) )
				return ( 1 );		/* confused?  edit_the_msg
							 * returns 1 if bad */

			batch = FALSE;			/* we've done it... * */
			edit_message = FALSE;

			Raw( ON );
			goto top_of_verify_loop;

		} else {
			while ( gets(very_long_buffer) != NULL )
				if ( fprintf(reply, "%s\n", very_long_buffer) == EOF ){
				        err = errno;
					printf( catgets(nl_fd,NL_SETN,3,"Sorry, fail to transmit : %s\n"), 
						error_description(err) );
					fclose( reply );
					unlink( filename );
					return( 1 );
				}	
		}
	}


#ifdef FORMS_MODE_SUPPORTED
	if ( copy_msg == FORM ) {
		sprintf( fname, "%s/%s%d", tmpdir, temp_form_file, getpid() );
		fclose( reply );			/* we can't retransmit a form! */

		if ( access(fname, ACCESS_EXISTS) != 0 ) {
			error( catgets(nl_fd,NL_SETN,4,"couldn't find forms file!") );
			return ( 0 );
		}

		unlink( filename );
		dprint( 4, (debugfile, "-- linking existing file %s to file %s --\n",
			   fname, filename) );
		link( fname, filename );
		unlink( fname );
	} else
#endif


	if ( copy_msg && !retransmit )			/* if retransmit we have it! */
		if ( edit_message ) {
			if ( copy_message(prefixchars, reply, noheader, NULL) ){
			         fclose( reply );
				unlink( filename );
				dprint( 1,(debugfile, 
					"Couldn't copy the message into edit buffer: %s",
					error_description(errno)) );
				return( 0 );
			}

			already_has_text = TRUE;	/* we just added it,
							 * right? */
		} else
			if ( copy_message("", reply, noheader, NULL) ){
			        fclose( reply );
				unlink( filename );
				dprint( 1, (debugfile, 
					"Couldn't copy the message into edit buffer: %s", 
					error_description(errno)) );
				return( 0 );
			}	

	if ( !batch && !retransmit && signature && copy_msg != FORM ) {

		if ( chloc(expanded_to, '!') == -1 && chloc(expanded_to, '@') == -1 ) {

			if ( strlen(local_signature) > 0 ) {

				if ( fprintf(reply, "\n--\n") == EOF ){	/* News 2.11 compatibility? */
		        		err = errno;
		        		dprint( 4, (debugfile, "Couldn't append signature file") );

					if ( mail_only )
						printf( catgets(nl_fd,NL_SETN,5,"Warning - couldn't append signature file: %s\r\n"),
							error_description(err) );
					else {
						error1( catgets(nl_fd,NL_SETN,6,"Warning - couldn't append signature file: %s"),
							error_description(err) );
						(void) sleep( 2 );
					}
				}	

				filename2[0] = '\0';		/* clear buffer */
				if ( local_signature[0] != '/' &&
					strlen(home)+strlen(local_signature) < LONG_FILE_NAME )
					sprintf(filename2, "%s/%s", home, local_signature);
				else
					strcpy( filename2, local_signature );

				if ( append(reply, filename2) ){

				        dprint( 4, (debugfile, 
					      "Couldn't append signature file completely"));

					if ( mail_only )
						printf( catgets(nl_fd,NL_SETN,7,"Warning - couldn't append signature file completely: %s\r\n"),
					  	error_description(errno) );
					else {
						error1( catgets(nl_fd,NL_SETN,8,"Warning - couldn't append signature file completely: %s"),
					  	error_description(errno) );
						(void) sleep( 2 );
					}
				}	

				already_has_text = TRUE;	/* added signature... */

			}

		} else {
			if ( strlen(remote_signature) > 0 ){

				if ( fprintf(reply, "\n--\n") == EOF ){	/* News 2.11 compatibility? */
		        		err = errno;
		        		dprint( 4, (debugfile, "Couldn't append signature file") );

					if ( mail_only )
						printf( catgets(nl_fd,NL_SETN,5,"Warning - couldn't append signature file: %s\r\n"),
							error_description(err) );
					else {
						error1( catgets(nl_fd,NL_SETN,6,"Warning - couldn't append signature file: %s"),
							error_description(err) );
						(void) sleep( 2 );
					}
				}	

				filename2[0] = '\0';		/* clear buffer */

				if ( remote_signature[0] != '/' &&
					strlen(home)+strlen(remote_signature) < LONG_FILE_NAME )
					sprintf(filename2, "%s/%s", home, remote_signature);
				else
					strcpy( filename2, remote_signature );

				if ( append(reply, filename2) ){

			       	 	dprint( 4, (debugfile, 
					     "Couldn't append signature file completely") );

					if ( mail_only )
						printf( catgets(nl_fd,NL_SETN,7,"Warning - couldn't append signature file completely: %s\r\n"), 
					 	 error_description(errno) );
					else {
						error1( catgets(nl_fd,NL_SETN,8,"Warning - couldn't append signature file completely: %s"), 
					  	error_description(errno) );
						(void) sleep( 2 );
					}
				}	

				already_has_text = TRUE;		/* added signature... */
			}

		}
	}

	if ( !retransmit && copy_msg != FORM )
		if ( reply != NULL )
			(void) fclose( reply );			/* on replies, it won't be
						 		 * open! */

	/*
	 *          Edit the message        
	 */

	if ( edit_message )
		create_readmsg_file();				/* for "readmsg" routine */

top_of_verify_loop:

	ch = edit_message ? 'e' : ' ';				/* drop through if needed */

	if ( !batch ) {
		do {
			if ( hp_terminal )
				softkeys_off();

			switch ( ch ) {
			case 'e':
				if ( edit_the_message(filename, already_has_text, 
						mail_only ? 0:1) ){
					cancelled_msg = TRUE;
					return ( 1 );
				}
				break;

			case 'h':
				if ( mail_only )
					batch_header_editor();
				else
					edit_headers();
				if ( no_to_header ) {
					set_error( catgets(nl_fd,NL_SETN,13, "mail not sent without To: header! "));
					no_to_header = 0;
					cancelled_msg = TRUE;
					return ( 1 );
				}
				break;

			default: 				/* do nothing */ ;

			}

			/*
			 *  ask that silly question again... 
			 */

			if ( (ch = verify_transmission(filename, &form)) == 'f' ) {
				cancelled_msg = TRUE;
				return ( 1 );
			}

		} while ( ch != 's' );


#ifdef FORMS_MODE_SUPPORTED
		if ( form == YES )
			if ( format_form(filename) < 1 ) {
				cancelled_msg = TRUE;
				return ( 1 );
			}
#endif


		if ( (reply = fopen(filename, "r")) == NULL ) {

			dprint( 1, (debugfile,
				   "Attempt to open file %s for reading failed with error %s (mail)\n",
				   filename, error_name(errno)) );

			if ( mail_only )
				printf( catgets(nl_fd,NL_SETN,14,"Could not open reply file (%s)\r\n"),
					error_name(errno) );
			else
				set_error1( catgets(nl_fd,NL_SETN,15,"Could not open reply file (%s)"),
					error_name(errno) );

			return ( 1 );
		}

	} else if ( (reply = fopen(filename, "r")) == NULL ) {
		dprint( 1, (debugfile,
			   "Attempt to open file %s for reading failed with error %s (mail)\n",
			   filename, error_name(errno)) );
		printf( catgets(nl_fd,NL_SETN,16,"Could not open reply file (%s)\n"), error_name(errno) );
		return ( 1 );
	}

	cancelled_msg = FALSE;				/* it ain't cancelled, is it? */

	/*
	 *  ask about bounceback if the user wants us to.... 
	 */

	
#ifdef BOUNCEBACK_ENABLED
	if ( uucp_hops(to) > bounceback && bounceback > 0 && copy_msg != FORM )
		if ( verify_bounceback() == TRUE ) {
			if ( strlen(cc) > 0 )
				strcat( expanded_cc, ", " );
			strcat( expanded_cc, bounce_off_remote(to) );
		}
#endif


	/*
	 *  write all header information into real_reply 
	 */

	sprintf( filename2, "%s/%s%d", tmpdir, temp_head_file, getpid() );

	/*
	 *  try to write headers to new temp file 
	 */

	dprint( 6, (debugfile, "Composition file='%s' and mail buffer='%s'\n",
		   filename, filename2) );


#ifdef ALLOW_BCC
	dprint( 2, (debugfile, "--\nTo: %s\nCc: %s\nBcc: %s\nSubject: %s\n---\n",
		   expanded_to, expanded_cc, expanded_bcc, subject) );
#else
	dprint( 2, (debugfile, "---\nTo: %s\nCc: %s\nSubject: %s\n---\n",
		   expanded_to, expanded_cc, subject) );
#endif


	real_reply = write_header_info( filename2, expanded_to, 
		expanded_cc, form == YES ); 

	strip_commas( expanded_to );
	strip_commas( expanded_cc );
	strip_commas( expanded_bcc );

	if ( !copy_message_across(reply, real_reply) || real_reply != NULL ){

		fclose( reply );
          	fclose( real_reply );

		if ( cc[0] != '\0' )				/* copies! */
		        sprintf( expanded_to, "%s %s", expanded_to, expanded_cc );


#ifdef ALLOW_BCC
		if ( bcc[0] != '\0') {
		        strcat( expanded_to, " " );
		        strcat( expanded_to, expanded_bcc );
		}
#endif


		if ( expand_tabs ){
		        unlink( filename );
			sprintf( very_long_buffer, "%s %s > %s 2>/dev/null", 
				 expand_cmd, filename2, filename );
			
			if (system_call( very_long_buffer, SH, 0L )) {
				if (!mail_only)
					set_error("unable to expand tabs - mail not sent !");
				else
					printf("unable to expand tabs - mail not sent !");
				return(1);
			}

			unlink(filename2);
			if (link(filename,filename2)) {
				/*unlink(filename);*/
				if (!mail_only)
					set_error("unable to expand tabs - mail not sent !");
				else
					printf("unable to expand tabs - mail not sent !");
				return(1);
			}
			/*unlink(filename);*/
		}

		if ( access(sendmail, EXECUTE_ACCESS) == 0


# ifdef SITE_HIDING
		        && !is_a_hidden_user(username) ) {
# else
			) {
# endif


			if ( strlen(sendmail)+strlen(smflags)+strlen(expanded_to)
			     +strlen(remove)+2*strlen(filename2)+17 < VERY_LONG_STRING )
				sprintf( very_long_buffer, "( (nohup %s %s %s ; %s %s) > /dev/null 2>&1 &) < %s",
			        	 sendmail, smflags, strip_parens(expanded_to),
			        	 remove, filename2, filename2 );
			else {
				if ( !mail_only )
				   set_error("too many recipients to send to - mail not sent !");
				else
				   printf("too many recipients to send to - mail not sent !\n\r");
				return(1);
			}
		} else {					/* use default mailer */
			if ( strlen(mailer)+strlen(expanded_to)
			     +strlen(remove)+2*strlen(filename2)+17 < VERY_LONG_STRING )
		        	sprintf( very_long_buffer, "( (nohup %s %s ; %s %s) > /dev/null 2>&1 & ) < %s",
		                 mailer, strip_parens(expanded_to),
			         remove, filename2, filename2 );
			else {
				if ( !mail_only )
				   set_error("too many recipients to send to - mail not sent !");
				else
				   printf("too many recipients to send to - mail not sent !\r\n");
				return(1);
			}
		}
	} else {

		/*
		 *  IT FAILED!!   Use another  mailer instead! 
		 */

	        fclose( real_reply );

		dprint( 3, (debugfile, "** write_header failed: %s\n",
			   error_name(errno)) );

		if ( cc[0] != '\0' )				/* copies! */
			sprintf( expanded_to, "%s %s", expanded_to, expanded_cc );


#ifdef ALLOW_BCC
		if ( bcc[0] != '0' )
		        sprintf( expanded_to, "%s %s", expanded_to, expanded_bcc );
#endif


		if ( strlen(mailx)+strlen(subject)+strlen(expanded_to)
		     +strlen(remove)+2*strlen(filename)+17 < VERY_LONG_STRING )
			sprintf( very_long_buffer, "( (nohup %s -s \"%s\" %s ; %s %s) > /dev/null 2>&1 & ) < %s",
		         mailx, subject, strip_parens(expanded_to),
			 remove, filename, filename );
		else {
			if ( !mail_only )
			   set_error("too many recipients to send to - mail not sent !");
			else
			   printf("too many recipients to send to - mail not sent !\n\r");
			return(1);
		}

		if ( mail_only )
			printf( catgets(nl_fd,NL_SETN,19,"Message sent using another mailer - %s\r\n"), mailx );
		else
			error1( catgets(nl_fd,NL_SETN,20,"Message sent using another mailer - %s"), mailx );

		(void) sleep( 2 );				/* ensure time to see this prompt */

		fclose( reply );
	}

	if ( mail_only ) {
		printf( catgets(nl_fd,NL_SETN,21,"sending mail...") );
		fflush( stdout );
	} else {
		PutLine0( LINES, 0, catgets(nl_fd,NL_SETN,21,"sending mail...") );
		CleartoEOLN();
	}

	if (system_call( very_long_buffer, SH, 0L )) {
		if (!mail_only)
			set_error("Unable to send mail - mail not sent !");
		else
			printf("Unable to send mail - mail not sent !");
		return(1);
	}

	/*
	 *  grab a copy if the user so desires... 
	 */

	if ( auto_cc )
		if ( !save_copy(subject, expanded_to, expanded_cc, 

#ifdef ALLOW_BCC
			expanded_bcc,
#endif

			filename, to) ){
		        dprint( 2, (debugfile, "Couldn't copy the outgoing message: %s",
			error_description(errno)));

			if ( mail_only )
				printf( catgets(nl_fd,NL_SETN,17,"Couldn't copy outgoing message to %s : %s\r\n"),
					filename, error_description(errno) );
			else
				error2( catgets(nl_fd,NL_SETN,18,"Couldn't copy outgoing message to %s : %s"),
					filename, error_description(errno) );

			sleep( 2 );
		}	

	if ( mail_only )
		printf( catgets(nl_fd,NL_SETN,23,"\rmail sent!      \n\r") );
	else
		set_error( catgets(nl_fd,NL_SETN,24,"Mail sent!") );

	return ( 1 );
}


#ifdef FORMS_MODE_SUPPORTED

int 
mail_form( address, subj )

	char            *address, 
			*subj;

{
	/*
	 *  copy the appropriate variables to the shared space...
	 */


	strcpy( subject, subj );
	strcpy( to, address );
	strcpy( expanded_to, address );

	return ( mail(FORM, NO, NO, NO) );
}

#endif


int
recall_last_msg( filename, copy_msg, cancelled_msg, already_has_text )

	char            *filename;
	int             copy_msg, 
			*cancelled_msg, 
			*already_has_text;

{
	/*
	 *  If filename exists and we've recently cancelled a message,
	 *  the ask if the user wants to use that message instead!  This
	 *  routine returns TRUE if the user wants to retransmit the last
	 *  message, FALSE otherwise...
	 */


	register int    retransmit = FALSE;
	char		ch;


	if ( access(filename, EDIT_ACCESS) == 0 && *cancelled_msg ) {
		Raw( ON );
		CleartoEOS();

		if ( hp_terminal )
			define_softkeys( YESNO );

		MoveCursor( LINES - 1, 0 );
		CleartoEOS();

		if ( copy_msg )
			PutLine1( LINES - 1, 0, catgets(nl_fd,NL_SETN,25,"Recall last kept message instead? (y/n) y%c"),
				  BACKSPACE );
		else
			PutLine1( LINES - 1, 0, catgets(nl_fd,NL_SETN,26,"Recall last kept message? (y/n) y%c"),
				  BACKSPACE );

		fflush( stdout );
		
		ch = yesno( 'y', TRUE );

		if ( ch == 'y' ) {
			retransmit = TRUE;
			*already_has_text = TRUE;
		} else if ( ch == eof_char ) {
			ClearLine( LINES - 2 );
			ClearLine( LINES - 1 );
			retransmit = eof_char;
		}

		fflush( stdout );

		*cancelled_msg = 0;
	}

	return ( retransmit );
}


int
verify_transmission( filename, form_letter )

	char           *filename;
	int            *form_letter;

{
	/*
	 *  Ensure the user wants to send this.  This routine returns
	 *  the character entered.  Modified compliments of Steve Wolf 
	 *  to add the'dead.letter' feature.
	 *  Also added form letter support... 
	 */


	FILE            *deadfd, 
			*messagefd;
	char            ch, 
			buffer[VERY_LONG_STRING], 
			fname[SLEN];


	if ( mail_only ) {
		if ( isatty(fileno(stdin)) ) {

	batch_reprompt:
			if ( strlen(to) == 0 ) {
				printf( "\n\r\n\r" );
				goto batch_msg_cancel;
			}

			if ( hp_terminal )
				define_softkeys( SEND );

			printf( catgets(nl_fd,NL_SETN,27,"\n\rYour options now are:\n\r") );
			printf( catgets(nl_fd,NL_SETN,28,"S)end the message, E)dit it again, change/add H)eaders or F)orget it\n\r"));
			printf( catgets(nl_fd,NL_SETN,29,"\n\rWhat is your choice? s%c"), BACKSPACE );
			fflush( stdin );			/* wait for answer! */
			fflush( stdout );

#ifdef SIGWINCH
			while((ch = (char)tolower(ReadCh())) == (char) NO_OP_COMMAND)
				;
#else
			ch = (char)tolower(ReadCh());
#endif

			if ( ch == 'f' ) {			/* forget this message! */
				printf( "Forget\n\r\n\r" );

				/*
				 *  try to save it as a dead letter file 
				 */
	batch_msg_cancel:
				sprintf( fname, "%s/%s", home, dead_letter );

				if ( (deadfd = fopen(fname, "a")) == NULL ) {
					dprint( 1, (debugfile,
						"\nAttempt to append to deadletter file '%s' failed: %s\n\r",
						 fname, error_name(errno)) );
					printf( catgets(nl_fd,NL_SETN,30,"Message not saved, Sorry.\n\r\n\r") );
					return ( 'f' );

				} else if ( (messagefd = fopen(filename, "r")) == NULL ) {
					dprint( 1, (debugfile,
						"\nAttempt to read reply file '%s' failed: %s\n\r",
					        filename, error_name(errno)) );
					printf( catgets(nl_fd,NL_SETN,30,"Message not saved, Sorry.\n\r\n\r") );
					fclose( deadfd );
					return ( 'f' );
				}

				/*
				 * if we get here we're okay for everything, right.
				 */

				while ( fgets(buffer, VERY_LONG_STRING, messagefd) != NULL )
					fputs( buffer, deadfd );

				fclose( messagefd );
				fclose( deadfd );

				printf( catgets(nl_fd,NL_SETN,32,"Message saved in file \"$HOME/%s\"\n\r\n\r"), 
					dead_letter);

				return ( 'f' );			/* forget it! */

			} else
				switch ( ch ) {
				case '\r':
				case '\n':
					ch = 's';

				case 's':
					printf( "Send\n\r" );
					break;

				case 'e':
					printf( "Edit the message\n\r" );
					break;

				case 'h':
					printf( "Edit the headers\n\r" );
					break;

				default:
					printf( catgets(nl_fd,NL_SETN,33,"\n\rI'm afraid I don't understand that - please answer again...\n\r"));
					goto batch_reprompt;
				}

			return ( ch );			/* send this whatever!*/
		} else
			return ( 's' );			/* SEND!               */

	} else if ( check_first ) {			/* used to check strlen(infile) > 0
					 	 	 * too? */
reprompt:

		ClearLine( LINES - 2 );
		ClearLine( LINES - 1 );
		ClearLine( LINES );


#ifdef FORMS_MODE_SUPPORTED
		softkeys_stat = *form_letter;
#endif

		if ( user_level == 0 ) {
			PutLine0( LINES - 2, 0, catgets(nl_fd,NL_SETN,34,"Please choose one of the following options by the first character") );
			PutLine1( LINES - 1, 0, catgets(nl_fd,NL_SETN,35,"E)dit the message, edit the H)eaders, S)end it, or F)orget it: s%c"),
			  BACKSPACE );
		}


#ifdef FORMS_MODE_SUPPORTED
		else if ( *form_letter == PREFORMATTED )
			PutLine1( LINES - 1, 0,
			   catgets(nl_fd,NL_SETN,36,"Choose: edit H)eaders, S)end, or F)orget : s%c"),
			   BACKSPACE );
		else if ( *form_letter == YES )
			PutLine1( LINES - 1, 0, catgets(nl_fd,NL_SETN,37,"Choose: E)dit form, edit H)eaders, S)end, or F)orget : s%c"),
			   BACKSPACE );
		else if ( *form_letter == MAYBE )
			PutLine1( LINES - 1, 0, catgets(nl_fd,NL_SETN,38,"Choose: E)dit msg, edit H)eaders, M)ake form, S)end, or F)orget : s%c"),
			   BACKSPACE );
#endif


		else
			PutLine1( LINES - 1, 0, catgets(nl_fd,NL_SETN,39,"Please choose: E)dit msg, edit H)eaders, S)end, or F)orget : s%c"),
			BACKSPACE );

		fflush( stdin );				/* wait for answer!      */
		fflush( stdout );

		if ( hp_terminal )
			define_softkeys( SEND );

		Raw( ON );				/* double check... testing only...*/

		ch = (char)tolower( ReadCh() );

		switch ( ch ) {
		case 'f':
			Write_to_screen( "Forget", 0, NULL, NULL, NULL );
			if ( user_level != 0 )
				set_error( catgets(nl_fd,NL_SETN,40,"Message kept - Can be restored at next F)orward, M)ail or R)eply "));
			break;

		case '\n':
		case '\r':
		case 's':
			Write_to_screen( "Send", 0, NULL, NULL, NULL );
			ch = 's';				/* make sure! */
			break;

		case 'm':


#ifdef FORMS_MODE_SUPPORTED
			if ( *form_letter == MAYBE ) {
				*form_letter = YES;

				switch ( check_form_file(filename) ) {
				case -1:
					return ( 'f' );

				case 0:
					*form_letter = MAYBE;	/* check later! */
					error( catgets(nl_fd,NL_SETN,41,"No fields in form!") );
					(void) sleep( 2 );
					return ( 'e' );

				default:
					goto reprompt;

				}

			} else {
#endif


				Write_to_screen( "%c", 1, 07, NULL, NULL );	/* BEEP */
				goto reprompt;	


#ifdef FORMS_MODE_SUPPORTED
			}
#endif


		case 'e':


#ifdef FORMS_MODE_SUPPORTED
			if ( *form_letter != PREFORMATTED ) {
				Write_to_screen( "Edit", 0, NULL, NULL, NULL );
				if ( *form_letter == YES )
					*form_letter = MAYBE;
			} else {
				Write_to_screen( "%c", 1, 07, NULL, NULL );	/* BEEP */
				(void) sleep( 1 );
				goto reprompt;		
			}
#else
			Write_to_screen( "Edit", 0, NULL, NULL, NULL );
#endif


			break;

		case 'h':
			Write_to_screen( "Headers", 0, NULL, NULL, NULL );
			break;

#ifdef SIGWINCH
		case NO_OP_COMMAND:
			goto reprompt;
#endif

		default:
			Write_to_screen( "%c", 1, 07, NULL, NULL );		/* BEEP */
			(void) sleep( 1 );
			goto reprompt;
		}

		return ( ch );

	} else

		return ( 's' );

}


FILE  *
write_header_info( filename, long_to, long_cc, form )

	char            *filename, 
			*long_to, 
			*long_cc;
	int             form;

{
	/*
	 *  Try to open filedesc as the specified filename.  If we can,
	 *  then write all the headers into the file.  The routine returns
	 *  'filedesc' if it succeeded, NULL otherwise.  Added the ability
	 *  to have backquoted stuff in the users .elmheaders file!
	 */


#ifdef SITE_HIDING
	char            buffer[SLEN];
	int             is_hidden_user;		/* someone we should know about?  */
#endif


	int             format_no_cr;		/* format_long() doesn't append CR */

	char           *get_arpa_date();


	if ( (filedesc = fopen(filename, "w")) == NULL ) {
		dprint( 1, (debugfile,
			   "Attempt to open file %s for writing failed! (write_header_info)\n",
			   filename) );
		dprint( 1, (debugfile, "** %s - %s **\n\n", error_name(errno),
			   error_description(errno)) );

		if ( mail_only )
			printf(catgets(nl_fd,NL_SETN,42,"Error %s encountered trying to write to %s\r\n"),
		       		error_name(errno), filename );
		else
			error2( catgets(nl_fd,NL_SETN,43,"Error %s encountered trying to write to %s"),
		        	error_name(errno), filename );

		(void) sleep( 2 );
		return ( NULL );			/* couldn't open it!! */
	}


#ifdef SITE_HIDING
	if ( (is_hidden_user = is_a_hidden_user(username)) ) {

		/*
		 *  this is the interesting part of this trick... 
		 */

		sprintf( buffer, "From %s!%s %s\n", HIDDEN_SITE_NAME,
			 username, get_ctime_date() );

		if ( fprintf(filedesc, "%s", buffer) == EOF )
		        return( NULL );

		dprint( 1, (debugfile, "\nadded: %s", buffer) );

		/*
		 *  so is this perverted or what? 
		 */
	}
#endif


	/*
	 *  Subject moved to top of headers for mail because the
	 *  pure System V.3 mailer, in its infinite wisdom, now
	 *  assumes that anything the user sends is part of the 
	 *  message body unless either:
	 *	 1. the "-s" flag is used (although it doesn't seem
	 *	    to be supported on all implementations??)
	 *	 2. the first line is "Subject:".  If so, then it'll
	 *	    read until a blank line and assume all are meant
	 *	    to be headers.
	 *  So the gory solution here is to move the Subject: line
	 *  up to the top.  I assume it won't break anyone elses program
	 *  or anything anyway (besides, RFC-822 specifies that the *order*
	 *  of headers is irrelevant). 
	 */

	/*
	 * sendmail doesn't like CR in header field and this inserts
	 * blank line in headers portion. So strip CR.
	 */

	format_no_cr = mail_only;
	mail_only = FALSE;			/* to strip CR	*/

	if ( fprintf(filedesc, "Subject: %s\n", subject) == EOF )
	        return( NULL );


	if ( fprintf(filedesc, "To: %s\n", format_long(long_to, (int)strlen("To: ")) ) == EOF ) 
	        return( NULL );

	if ( fprintf(filedesc, "Date: %s\n", get_arpa_date()) == EOF )
	        return( NULL );


	if ( !equal(dflt_full_name, full_username) ) {
#ifndef DONT_ADD_FROM
# ifdef SITE_HIDING
		if ( is_hidden_user )
			if ( fprintf(filedesc, "From: %s <%s!%s!%s>\n", full_username,
				hostname, HIDDEN_SITE_NAME, username) == EOF )
		        	return( NULL );
		else
# endif

# ifdef  INTERNET_ADDRESS_FORMAT
#  ifdef  USE_DOMAIN
			if ( fprintf(filedesc, "From: %s <%s@%s%s>\n", full_username,
				username, hostname, DOMAIN) == EOF )
		        	return( NULL );
#  else
		if ( fprintf(filedesc, "From: %s <%s@%s>\n", full_username,
			username, hostname) == EOF )
	        	return( NULL );
#  endif
# else
		if ( fprintf(filedesc, "From: %s <%s!%s>\n", full_username,
			hostname, username) == EOF )
	        	return( NULL );
# endif
#endif
		if ( fprintf(filedesc, "Full-Name: %s\n", full_username) == EOF )
	        	return( NULL );
	}

	if ( cc[0] != '\0' )
		if ( fprintf(filedesc, "Cc: %s\n", format_long(long_cc,
			(int)strlen("Cc: "))) == EOF )
	                return( NULL );

	if ( strlen(action) > 0 )
		if ( fprintf(filedesc, "Action: %s\n", action) == EOF )
		        return( NULL );

	if ( strlen(priority) > 0 )
		if ( fprintf(filedesc, "Priority: %s\n", priority) == EOF )
		        return( NULL );

	if ( strlen(expires) > 0 )
	        if ( fprintf(filedesc, "Expires: %s\n", expires) == EOF )
		        return( NULL );

	if ( strlen(reply_to) > 0 )
		if ( fprintf(filedesc, "Reply-To: %s\n", format_long(
		     expanded_reply_to, (int)strlen("Reply-To: "))) == EOF )
		        return( NULL );

	if ( strlen(in_reply_to) > 0 )
		if ( fprintf(filedesc, "In-Reply-To: %s\n", in_reply_to) == EOF)
		        return( NULL );

	if ( strlen(user_defined_header) > 0 ) {
		if ( fprintf(filedesc, "%s\n", user_defined_header) == EOF )
		        return( NULL );
		user_defined_header[0] = '\0';
	}

	/*
	 * return the value of mail_only 
	 */

	mail_only = format_no_cr;

	if  ( add_mailheaders(filedesc) )
	        return( NULL );


#ifdef FORMS_MODE_SUPPORTED
	if ( form )
	        if ( fprintf(filedesc, "Content-Type: mailform\n") == EOF )
		        return( NULL );
#endif


	if ( fprintf(filedesc, "Mailer: Elm [%s]\n\n", revision_id) == EOF )
	        return( NULL );

	return ( (FILE *) filedesc );
}


int
copy_message_across( source, dest )

	FILE            *source, 
			*dest;

{
	/*
	 *  copy the message in the file pointed to by source to the
	 *  file pointed to by dest.  
	 */


#ifdef ENCRYPTION_SUPPORTED
	int             crypted = FALSE;		/* are we encrypting?  */
	int             encoded_lines = 0;		/* # lines encoded     */
#endif


	char            buffer[VERY_LONG_STRING];		/* file reading buffer */

	while ( fgets(buffer, VERY_LONG_STRING, source) != NULL ) {
		if ( buffer[0] == '[' ) {


#ifdef ENCRYPTION_SUPPORTED
			if ( first_word(buffer, START_ENCODE) )
				crypted = TRUE;
			else if ( first_word(buffer, END_ENCODE) )
				crypted = FALSE;
			else
#endif


			if ( first_word(buffer, DONT_SAVE) )
				continue;		/* next line? */
		}


#ifdef ENCRYPTION_SUPPORTED
		else if ( crypted ) {

			if ( !gotten_key++ )
				getkey( ON );
			else if ( !encoded_lines++ )
				get_key_no_prompt();	/* reinitialize.. */

			encode( buffer );
		}
#endif

		if ( fputs(buffer, dest) == EOF )
		        return( 1 );
	}

	return( 0 );
}


#ifdef BOUNCEBACK_ENABLED


int
verify_bounceback()

{
	/*
	 *  Ensure the user wants to have a bounceback copy too.  (This is
	 *  only called on messages that are greater than the specified 
	 *  threshold hops and NEVER for non-uucp addresses.... Returns
	 *  TRUE iff the user wants to bounce a copy back.... 
	 */


	char		ch;


	if ( mail_only ) {
		printf( catgets(nl_fd,NL_SETN,44,"Would you like a copy \"bounced\" off the remote? (y/n) ") );
		CleartoEOLN();
		printf( "n%c", BACKSPACE );
		fflush( stdin );				/* wait for answer! */
		fflush( stdout );

		while( 1 ) {
			ch = (char)tolower( ReadCh() );

			if ( ch != 'y' && ch != 'n' )
				(void) putchar( (char) 007 );	/* BEEP ! */
			else
				break;
		}

		if ( ch != 'y' ) {
			printf( "No\n\r" );
			return ( FALSE );
		} else
			printf( catgets(nl_fd,NL_SETN,45,"Yes - Bounceback included\n\r") );

	} else {
		MoveCursor( LINES, 0 );
		CleartoEOLN();
		PutLine1( LINES, 0,
		      catgets(nl_fd,NL_SETN,46,"\"Bounce\" a copy off the remote machine? (y/n) n%c"),
	   	      BACKSPACE );
		fflush( stdin );				/* wait for answer! */
		fflush( stdout );

		while( 1 ) {
			ch = (char)tolower( ReadCh() );

			if ( ch != 'y' && ch != 'n' )
				(void) putchar( (char) 007 );	/* BEEP ! */
			else
				break;
		}

		if ( ch != 'y' ) {
			Write_to_screen( "No", 0, NULL, NULL, NULL );
			fflush( stdout );
			return ( FALSE );
		}

		Write_to_screen( "Yes!", 0, NULL, NULL, NULL );
		fflush( stdout );
	}

	return ( TRUE );
}

#endif
