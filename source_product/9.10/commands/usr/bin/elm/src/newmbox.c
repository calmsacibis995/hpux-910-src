/**			newmbox.c			**/

/*
 *  @(#) $Revision: 72.4 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  read new mailbox file, or specified mailbox.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <signal.h>
#include <termio.h>

#include "headers.h"


#ifdef NLS
# define NL_SETN  28
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


extern int	num_char;

int
newmbox( status, resync, main_screen )

	int             status,
			resync, 
			main_screen;

{
	/*
	 *  Read a new mailbox file or resync on current file.
         *
	 *  Values of status and what they mean;
         *
	 *	status = 0	- changing mailboxes from within program
	 * 	status = 1	- read default mailbox or infile for the 
	 *		          first time
	 *      status = 2	- read existing mailbox, new mail arrived
	 *      status = 3	- resync on existing mailbox...
  	 *
	 *  resync is TRUE iff we know the current mailbox has changed.  If
	 *  it's set to true this means that we MUST READ SOMETHING, even 
	 *  if it's the current mailbox again!!
         *
	 *  main_screen simply tells where the counting line should be.
  	 *
	 */


	int             switching_from_default = 0;
	int             iterations = 0;			/* for dealing with the
							 * '?' answer */
	int		back_to_dft = 2;		/* case of file was removed */
	register int	i;
	char            buff1[LONG_FILE_NAME],
			buff2[LONG_FILE_NAME],
			*strp;
	FILE		*fpipe;
#ifdef SIGWINCH
	struct winsize	w_before, w_after;
	void		(*wstat) ();
#endif


	if ( mbox_specified == 0 && status == 0 )
		switching_from_default++;

	if ( status > 0 ) {
		if ( status == 1 && infile[0] == 0 ) {

			/*
			 * Subtlety - check to see if there's another
			 * instantiation of Elm (e.g. if the /tmp file is in
			 * use).  If so, DIE! 
			 */

			sprintf( infile, "%s/%s%s", tmpdir, temp_mbox, username );

			if ( access(infile, ACCESS_EXISTS) != -1 ) {
				fprintf( stderr, catgets(nl_fd,NL_SETN,1,"     Another elm is already reading this mail!\n\r"));
				fprintf( stderr, catgets(nl_fd,NL_SETN,2,"\n\r     [if this is in error then you'll need to remove '%s/%s%s']\n\r"),
					 tmpdir, temp_mbox, username );

				if ( has_transmit )
					transmit_functions( OFF );

				exit( 1 );
			}

			sprintf( infile, "%s%s", mailhome, username );
		}

		if ( infile[0] == 0 )	/* no filename yet?? */
			sprintf( infile, "%s%s", mailhome, username );

		if ( user_access(infile, READ_ACCESS) != 0 ) {
		
			int             err = errno;
			char            tempfile[LONG_FILE_NAME];

			if ( first_word(infile, mailhome) ) {

				/*
				 * incoming mailbox with no messages... 
				 */

				if ( status == 3 ) {
					current = 0;
					header_page = 0;
					last_header_page = -1;
					message_count = 0;
					mailfile_size = 0L;
					deleted_at_cancel = 0;
					clear_error();
					clear_central_message();
				}

				/*
				 * remove temporary file to keep it in sync
				 * with mailbox.
				 */
				sprintf(tempfile, "%s/%s%s", tmpdir, temp_mbox, username);
#ifdef DEBUG
				if (unlink(tempfile) < 0)
				    dprint(3, (debugfile, "newmbox(): Cannot unlink %s\n",
					       tempfile));
#endif
				return (1);
			}

			error2( catgets(nl_fd,NL_SETN,3,"Can't open mailbox '%s' (%s)"), infile,
			        error_description(err) );

			Raw( OFF );

			if ( has_transmit )
				transmit_functions( OFF );

			exit( 1 );
		}
		
	} else {		/* get name of new mailbox! */
		MoveCursor( LINES - 3, 30 );
		CleartoEOS();
		PutLine0( LINES - 3, COLUMNS - 40, 
			  catgets(nl_fd,NL_SETN,4,"(Use '?' to list your folders)") );
		show_last_error();

ask_again:
		buff1[0] = '\0';

		if ( iterations++ == 0 ) {
			PutLine0( LINES - 2, 0, catgets(nl_fd,NL_SETN,5,"Name of new mailbox: ") );
			num_char = strlen(catgets(nl_fd,NL_SETN,5,"Name of new mailbox: ") );
			(void) optionally_enter( buff1, LINES - 2, num_char, FALSE );
			ClearLine( LINES - 2 );
		} else {
			printf( catgets(nl_fd,NL_SETN,6,"\n\rName of new mailbox: ") );
			(void) optionally_enter( buff1, -1, -1, FALSE );
		}

		strp = buff1;
		remove_spaces( strp );
		strcpy( buff1, strp );

		if ( equal(buff1, "?") ) {
			Raw( OFF );
			list_folders();
			Raw( ON );
			goto ask_again;
		}

		if ( ((chloc(buff1, ' ') != -1)   ||
		      (chloc(buff1, '\t') != -1)) &&
		      (chloc(buff1, '\'') == -1)  &&
		      (chloc(buff1, '\"') == -1)     ) {
			error( catgets(nl_fd,NL_SETN,29,"Invalid file name.  Mailbox can not be changed.") );
			iterations = 0;
			goto ask_again;
		}

		if ( (buff1[0] == '\''                  &&
		      buff1[strlen(buff1) - 1] != '\'') ||
		     (buff1[0] == '\"'                  &&
		      buff1[strlen(buff1) - 1] != '\"')    ) {
			error( catgets(nl_fd,NL_SETN,29,"Invalid file name.  Mailbox can not be changed.") );
			iterations = 0;
			goto ask_again;
		}

#ifdef SIGWINCH
		ioctl(0, TIOCGWINSZ, &w_before);
#endif
		sprintf( buff2, "/bin/echo %s", buff1 );
		if ( (fpipe = popen(buff2, "r")) == NULL ) {
			error( catgets(nl_fd,NL_SETN,30,"Popen error.  Mailbox can not be changed.") );
			iterations = 0;
			goto ask_again;
		}
#ifdef SIGWINCH
		wstat = signal(SIGWINCH, SIG_IGN);
#endif
		fgets( buff1, LONG_FILE_NAME, fpipe );
		pclose( fpipe );
#ifdef SIGWINCH
		ioctl(0, TIOCGWINSZ, &w_after);
		if ((w_before.ws_row != w_after.ws_row) ||
				(w_before.ws_col != w_after.ws_col))
			setsize(w_after.ws_row, w_after.ws_col);
		signal(SIGWINCH, wstat);
#endif
		no_ret( buff1 );

		if ( equal(buff1, "!") )		/* go to mailbox */
			sprintf( buff1, "%s%s", mailhome, username );

		sprintf( buff2, "%s%s", mailhome, username );

		if ( equal(buff1, buff2) ) {
			resync = FALSE;
			buff1[0] = '\0';
		}

		if ( buff1[0] == 0 || !expand_filename(buff1) ) {

			if ( buff1[0] ) {
				show_last_error();
				(void) sleep( 2 );
			} else
				if ( equal(buff2, infile) )
					resync = FALSE;

			if ( resync && (( mbox_specified && saved_at_cancel )
				      ||( mbox_specified && !file_changed )
				      ||( !mbox_specified && keep_in_incoming )))
				strcpy( buff1, infile );
			else {
				if ( ! mbox_specified ) {
					sprintf( buff1, "%s/%s%s", tmpdir, temp_mbox, username );
					unlink( buff1 );
				} else
					mbox_specified = FALSE;

				current = 0;
				header_page = 0;
				last_header_page = -1;
				message_count = 0;
				mailfile_size = 0L;
				infile[0] = '\0';

				 /* get egid 'mail' */
				setresgid( -1, egroupid, egroupid );

				return (back_to_dft);
			}
		}

		if ( equal(buff1, infile) && !resync ) {
			error( catgets(nl_fd,NL_SETN,8,"already reading that mailbox!") );
			return ( FALSE );
		}
		
		if ( user_access(buff1, READ_ACCESS) != 0 ) {

			int             err = errno;

			dprint( 2, (debugfile,
			        "Can't open mailbox %s in newmbox\n", buff1) );
			dprint( 2, (debugfile, "** %s - %s **\n",
				   error_name(err), error_description(err)) );
			error2( catgets(nl_fd,NL_SETN,9,"Can't open file '%s': %s"), buff1, 
				error_description(err) );
			iterations = 0;		/* we can still rely on the screen
					 	 * being intact at this point... */
			goto ask_again;
		}

		if ( resync && file_changed && equal(buff1, infile) )
			PutLine0( LINES - 3, COLUMNS - 40, 
				  catgets(nl_fd,NL_SETN,10,"Resynchronizing file") );
		else
			PutLine1( LINES - 3, COLUMNS - 40, 
				  catgets(nl_fd,NL_SETN,11,"Mailbox: %s"), buff1 );

		CleartoEOLN();
		strcpy( buff2, infile );
		strcpy( infile, buff1 );

		mbox_specified = 1;
	}

	if ( status == 3 )
		switching_from_default = TRUE;

	if ( switching_from_default ) {		/* we need to remove the tmp file */
		sprintf( buff1, "%s/%s%s", tmpdir, temp_mbox, username );

		if ( access(buff1, ACCESS_EXISTS) != -1 )	/* is it there at all? */
			if ( unlink(buff1) != 0 ) {
				error1( catgets(nl_fd,NL_SETN,12,"Sorry, but I can't seem to unlink your temp mail file [%s]\n\r"),
				  error_name(errno) );
				silently_exit();
			}
	}

	clear_error();
	clear_central_message();

	header_page = 0;
	deleted_at_cancel = 0;

	if ( mailfile != NULL )
		(void) fclose( mailfile );	/* close it first, to avoid
						 * too many open */

	if ( mbox_specified ) 
		mailfile = user_fopen( infile, "r" );
	else
		mailfile = fopen( infile, "r" );

	if ( mailfile == NULL)
		message_count = 0;
	else if ( !(status == 2 && mailfile_size < bytes(infile)) ) {

		if ( status != 3 && status != 2 )
			current = 0;	 	/* new mail file! */

		save_file_stats( infile );
		message_count = read_headers( FALSE, main_screen, (status==0 ? 1:0) );

		if ( message_count == 0 && status == 0 ) {
			strcpy( infile, buff2 );
			iterations = 0;
			goto ask_again;
		}

		if ( current > message_count )
		        current = message_count;

	} else {		/* resync with current mail file */
		save_file_stats( infile );
		message_count = read_headers( TRUE, main_screen, 0 );

	}

	if ( status != 2 )
		selected = 0;	/* we don't preselect new mailboxes, boss! */

	sort_mailbox( message_count, current ? 0:(point_to_new ? 0:1), 1 );

	if ( point_to_new && message_count && current == 0 ) {

		for ( i = 0; i < message_count; i++ )
			if ( header_table[i].status & NEW ) {
				current = i + 1;
				break;
			}

		if ( i >= message_count )
			current = 1;

		get_page( current );
	}

	return ( TRUE );
}


int
read_headers(rereading, main_screen, change_mbox)

	int             rereading, 
			main_screen,
			change_mbox;

{
	/*
	 *  Reads the headers into the header_table structure and leaves
	 *  the file rewound for further I/O requests.   If the file being
	 *  read is the default mailbox (ie incoming) then it is copied to
	 *  a temp file and closed, to allow more mail to arrive during 
	 *  the elm session.  If 'rereading' is set, the program will copy
	 *  the status flags from the previous data structure to the new 
	 *  one if possible.  This is (obviously) for re-reading a mailfile!
	 */


	FILE            *temp;
	struct header_rec *tmphdr;
	char            buffer[VERY_LONG_STRING], 
			temp_filename[SLEN];
	long            fbytes = 0L, 
			line_bytes = 0L;
	register int    line = 0, 
			count = 0, 
			subj = 0, 
			copyit = 0, 
			in_header = 1;
	int             count_x, 
			count_y = 17, 
			err;
	int             in_to_list = FALSE, 
			forwarding_mail = FALSE, 
			first_line = TRUE;


	static int      first_read = 0;

	if ( !first_read++ ) {
		ClearLine( LINES - 1 );
		ClearLine( LINES );
		if ( rereading )
			PutLine2( LINES, 0, catgets(nl_fd,NL_SETN,13,"Reading in %s, message: %d"), 
				  infile, message_count );
		else
			PutLine1( LINES, 0, 
				  catgets(nl_fd,NL_SETN,14,"Reading in %s, message: 0"), infile);
		count_x = LINES;
		count_y = 22 + strlen( infile );
	} else {
		count_x = LINES - 2;
		if ( main_screen )
			PutLine0( LINES - 2, 0, catgets(nl_fd,NL_SETN,15,"Reading message: 0") );
		else {
			PutLine0( LINES, 0, "\n" );
			PutLine0( LINES, 0, catgets(nl_fd,NL_SETN,15,"Reading message: 0") );
			count_x = LINES;
		}
	}

	if ( mbox_specified == 0 ) {
		lock( INCOMING );	/* ensure no mail arrives while we do this! */
		sprintf( temp_filename, "%s/%s%s", tmpdir, temp_mbox, username );
		if ( !rereading ) {
			if ( access(temp_filename, ACCESS_EXISTS) != -1 ) {

				/*
				 * The temp file already exists? 
				 * Looks like a potential clash of processes
				 * on the same file! 
				 */

				unlock();	/* so remove lock file! */
				error( catgets(nl_fd,NL_SETN,17,"The temp mailbox already exists !!"));
				(void) sleep( 2 );
				error(catgets(nl_fd,NL_SETN,18,"Leave without tampering with it now..."));
				silently_exit();	/* leave without
							 * tampering with it! */
			}

			if ( (temp = fopen(temp_filename, "w")) == NULL ) {
				err = errno;
				unlock();	/* remove lock file! */
				Raw( OFF );
				Write_to_screen( catgets(nl_fd,NL_SETN,19,"\nCouldn't open file %s for use as temp mailbox;\n"),
				     1, temp_filename, NULL, NULL );
				Write_to_screen( "** %s - %s **\n", 2,
				     error_name(err), error_description(err), NULL );
				dprint( 1, (debugfile,
					"Error: Couldn't open file %s as temp mbox.  errno %s (%s)\n",
					temp_filename, error_name(err), "read_headers") );
				leave( err );
			}

			get_mailtime();
			copyit++;
			chown( temp_filename, userid, groupid );
			chmod( temp_filename, 0700 );	/* shut off file for
							 * other people! */
		} else {
			if ( (temp = fopen(temp_filename, "a")) == NULL ) {
				err = errno;
				unlock();	/* remove lock file! */
				Raw( OFF );
				Write_to_screen( catgets(nl_fd,NL_SETN,20,"\nCouldn't reopen file %s for use as temp mailbox;\n"),
				   1, temp_filename, NULL, NULL );
				Write_to_screen( "** %s - %s **\n", 2,
				   error_name(err), error_description(err), NULL );
				dprint( 1, (debugfile,
					"Error: Couldn't reopen file %s as temp mbox.  errno %s (%s)\n",
					temp_filename, error_name(err), "read_headers") );
				emergency_exit();
			}

			copyit++;
		}
	}

	if ( rereading ) {
		if ( fseek(mailfile, mailfile_size, 0) == -1 ) {
			err = errno;
			Write_to_screen( catgets(nl_fd,NL_SETN,21,"\nCouldn't seek to %ld (end of mailbox) in %s!\n"), 2,
				mailfile_size, infile, NULL );
			Write_to_screen( "** %s - %s **\n", 2,
				error_name(err), error_description(err), NULL );
			dprint( 1, (debugfile,
				"Error: Couldn't seek to end of mailbox %s: (offset %ld) Errno %s (%s)\n",
				infile, mailfile_size, error_name(err), "read_headers") );
			emergency_exit();
		}

		count = message_count;			/* next available  */
		fbytes = mailfile_size;			/* start correctly */

		if ( message_count > 0 )
			line = header_table[message_count - 1].lines;
		else
			line = 0;
	}

	/*
	 *  find the size of the mailbox then unlock the file 
	 */

	mailfile_size = bytes( infile );
	if ( mbox_specified == 0 )
		unlock();

	/*
	 *  now let's copy it all across accordingly... 
	 */

	while ( fbytes < mailfile_size ) {
		
		if ( fgets(buffer, VERY_LONG_STRING, mailfile) == NULL )
			break;

		if ( fbytes == 0L || first_line ) {	/* first line of file... */
			if ( !mbox_specified ) {
				if ( first_word(buffer, "Forward to ") ) {
					set_central_message( catgets(nl_fd,NL_SETN,22,"Mail being forwarded to %s"),
						(char *) (buffer + 11) );
					forwarding_mail = TRUE;
				}
			}

			/*
			 *  flush leading blank lines before next test... 
			 */

			if (buffer[0]=='\n' && buffer[1]=='\0') {
				fbytes++;
				continue;
			} else
				first_line = FALSE;

			if ( !real_from(buffer, (struct header_rec *)0) && !forwarding_mail ) {
				if ( change_mbox == 0 ) {
				  PutLine0( LINES, 0, catgets(nl_fd,NL_SETN,23,"\n\rMail file is corrupt!!  Can't read it!!\n\r\n\r"));
				  fflush( stderr );
				  dprint(1, (debugfile,
				       "\n\n**** First mail header is corrupt!! ****\n\n"));
				  dprint( 1, (debugfile, "Line is;\n\t%s\n\n", buffer) );
				  mail_only++;		/* to avoid leave() cursor
				  			 * motion */
				  leave( 1 );
				} else {
				  error( catgets(nl_fd,NL_SETN,28,"Mail file is corrupt!!  Can't read it!!") );
				  dprint(1, (debugfile,
				       "\n\n**** First mail header is corrupt!! ****\n\n"));
				  dprint( 1, (debugfile, "Line is;\n\t%s\n\n", buffer) );
				  return( 0 );
				}

			}
		}

		if ( copyit ) 
			if ( fputs(buffer, temp) == EOF ){
			        err = errno;
		                error2( catgets(nl_fd,NL_SETN,24,"Couldn't copy %s file to temp mailbox: %s"), 
				        temp_filename, error_description(err) );
				(void) sleep( 2 );
				error( catgets(nl_fd,NL_SETN,25,"Sorry, leave without any change with mailbox..."));
				silently_exit();
			}

		line_bytes = (long) strlen( buffer );
		line++;

		if ( real_from(buffer, (struct header_rec *)0) ) {

			/*
			 *  try to allocate new headers, if needed... 
			 */

			if ( count >= max_headers ) {
			    max_headers += KLICK;

			    tmphdr = (struct header_rec *)
			 		my_realloc((void *)header_table,
						   max_headers *
					           sizeof(struct header_rec),
						   "new headers");
			    header_table = tmphdr;
			}

			/*
			 * Now make sure the initializing !
			 */

			header_table[count].lines = 0;
			header_table[count].status = 0;
			header_table[count].status2 = NEW;
			header_table[count].index_number = 0;
			header_table[count].offset = 0L;
			header_table[count].from[0]='\0';
			header_table[count].to[0]='\0';
			header_table[count].messageid[0]='\0';
			header_table[count].dayname[0]='\0';
			header_table[count].month[0]='\0';
			header_table[count].day[0]='\0';
			header_table[count].year[0]='\0';
			header_table[count].time[0]='\0';
			header_table[count].subject[0]='\0';

			if ( real_from(buffer, &header_table[count]) ) {
				header_table[count].offset = fbytes;
				header_table[count].index_number = count + 1;

				if ( !rereading || count >= message_count )

					/*
					 * default status! 
					 */

					header_table[count].status = VISIBLE;	

				strcpy( header_table[count].subject, "" );	/* clear  */

				if ( count )
					if ( rereading )
						header_table[count - 1].lines = line - 1;
					else
						header_table[count - 1].lines = line;

				if ( new_msg(header_table[count]) )
					header_table[count].status |= NEW;/* new message! */

				count++;
				subj = 0;
				line = 0;
				in_header = 1;
				PutLine1( count_x, count_y, "%d", count );
			}

		} else if ( in_header ) {

			if ( first_word(buffer, ">From ") )
				forwarded(buffer, &header_table[count - 1]);/*return addr */

			else if ( first_word(buffer, "Subject:") ||
				  first_word(buffer, "Subj:") ||
				  first_word(buffer, "Re:") ) {

				if ( !subj++ ) {
					remove_first_word( buffer );
					copy_sans_escape(
					(unsigned char *)header_table[count - 1].subject, 
					(unsigned char *)buffer, STRING );
					remove_possible_trailing_spaces(header_table[count
									  - 1].subject);
				}

			} else if ( first_word(buffer, "Status:") ) {
				remove_first_word( buffer );
				if ( in_string(buffer, "R") )
					header_table[count - 1].status2 = 0;
				else
					header_table[count - 1].status2 |= URGENT;
				if ( (header_table[count - 1].status & NEW) &&
				     (header_table[count - 1].status2 == 0) )
					clearit( header_table[count - 1].status, NEW );
			} else if ( first_word(buffer, "From:") || first_word(buffer, ">From:"))
				parse_arpa_from( buffer, header_table[count - 1].from );

			else if ( first_word(buffer, "Message-Id:") ||
				 first_word(buffer, "Message-ID:") ) {
				buffer[strlen(buffer) - 1] = '\0';
				strcpy( header_table[count - 1].messageid,
				        (char *) buffer + 12 );

			} else if ( first_word(buffer, "Expires:") )
				process_expiration_date( (char *) buffer + 9,
					 &(header_table[count - 1].status) );

			/*
			 *  when it was sent... 
			 */

			else if ( first_word(buffer, "Date:") )
				parse_arpa_date( buffer, &header_table[count - 1] );

			/*
			 *  some status things about the message... 
			 */

			else if ( first_word(buffer, "Priority:") ||
				  first_word(buffer, "Importance: 2") )
				header_table[count - 1].status |= URGENT;

			else if ( first_word(buffer, "Sensitivity: 2") )
				header_table[count - 1].status |= PRIVATE;

			else if ( first_word(buffer, "Sensitivity: 3") )
				header_table[count - 1].status |= CONFIDENTIAL;

			else if ( first_word(buffer, "Content-Type: mailform") )
				header_table[count - 1].status |= FORM_LETTER;

			else if ( first_word(buffer, "Action:") )
				header_table[count - 1].status |= ACTION;

			/*
			 *  next let's see if it's to us or not... 
			 */

			else if ( first_word(buffer, "To:") ) {
				in_to_list = TRUE;
				header_table[count - 1].to[0] = '\0';	/* nothing yet */
				figure_out_addressee( (char *) buffer + 3,
						header_table[count - 1].to );

			} else if ( buffer[0] == LINE_FEED || buffer[0] == '\0' ) {
				if ( in_header ) {
					in_header = 0;	/* in body of message! */
					fix_date( &header_table[count - 1] );
				}

			} else if ( in_to_list == TRUE ) {
				if ( whitespace(buffer[0]) )
					figure_out_addressee( buffer, 
						header_table[count - 1].to );
				else
					in_to_list = FALSE;
			}
		}

		fbytes += (long) line_bytes;
	}

	header_table[count > 0 ? count - 1 : count].lines = line + 1;

	if ( mbox_specified == 0 ) {
		fclose( mailfile );
		mailfile = NULL;
		fclose( temp );

		if ( (mailfile = fopen(temp_filename, "r")) == NULL ) {
			err = errno;
			MoveCursor( LINES, 0 );
			Raw( OFF );
			Write_to_screen( catgets(nl_fd,NL_SETN,27,"\nCouldn't reopen %s as temp mail file;\n"),
				1, temp_filename, NULL, NULL );
			Write_to_screen( "** %s - %s **\n", 2, error_name(err),
					 error_description(err), NULL );
			dprint( 1, (debugfile,
				"Error: Reopening %s as temp mail file failed!  errno %s (%s)\n",
			        temp_filename, error_name(errno), "read_headers") );
			leave( errno );
		}
	} else
		rewind( mailfile );

	if ( count == 0 && change_mbox == 1 )
		error("no messages in the folder");

	return ( count );

}
