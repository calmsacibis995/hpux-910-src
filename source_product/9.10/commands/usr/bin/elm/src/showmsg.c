/** 			showmsg.c			**/

/*
 *  @(#) $Revision: 72.5 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  This file contains all the routines needed to display the specified
 *  message.
 *
 */


#include <signal.h>
#include <termio.h>
#include <sys/wait.h>

#include "headers.h"


#ifdef NLS
# define NL_SETN  39
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


int             pipe_abort = FALSE;	/* did we receive a SIGNAL(SIGPIPE)? */

extern int      lines_displayed,	/* defined in "builtin"  */
		total_lines_to_display, /* total line num of msg */
                lines_put_on_screen;	/* ditto too!            */


int
show_msg( number )

	int             number;

{
	/*
	 *   display number'th message.  Get starting and ending lines
	 *   of message from headers data structure, then fly through
	 *   the file, displaying only those lines that are between the
	 *   two!
	 *	Returns non-zero iff the screen was changed, or the
	 *   character pressed at the 'end-of-screen' prompt (to be
	 *   processed via process_showmsg_cmd()).
	 */


	dprint( 8, (debugfile, "show_msg called\n") );

	if ( number > message_count ) {
		error1( catgets(nl_fd,NL_SETN,1,"Only %d messages!"), message_count );
		return ( 0 );
	} else if ( number < 1 ) {
		error( catgets(nl_fd,NL_SETN,2,"you can't read THAT message!") );
		return ( 0 );
	}

	clearit( header_table[number - 1].status, NEW );	/* it's been read now! */

	return ( show_message(header_table[number - 1].lines,
			     header_table[number - 1].offset, number) );
}


int
show_message( lines, file_loc, msgnumber )

	int             lines, 
			msgnumber;
	long            file_loc;

{
	/*
	 *   Show the indicated range of lines from mailfile
	 *   for message 'msgnumber' by using 'display'
	 *   Returns non-zero iff screen was altered, or the char
	 *   pressed (see previous routine header comment).
	 */


	int             retval;


	dprint( 9, (debugfile, "show_message(%d,%ld,%d)\n",
		   lines, file_loc, msgnumber) );

	if ( fseek(mailfile, file_loc, 0) == -1 ) {
		dprint( 1, (debugfile,
		"Error: seek %d bytes into file, errno %s (show_message)\n",
			   file_loc, error_name(errno)) );
		error2( catgets(nl_fd,NL_SETN,3,"ELM failed seeking %d bytes into file (%s)"),
		       file_loc, error_name(errno) );
		return ( 0 );
	}

#ifdef DEBUG
	if ( feof(mailfile) )
		dprint( 1, (debugfile, "\n*** seek put us at END OF FILE!!! ***\n") );
#endif

	/*
	 * next read will get 'this' line so must be at end of previous 
	 */

	if ( first_word(pager, "builtin") || first_word(pager, "internal") ){

		if ( hp_terminal )
			define_softkeys( READ );

		OnlcrSwitch( ON );

		retval = display( lines, msgnumber );

		OnlcrSwitch( OFF );
	} else {
		Raw( OFF );

		if ( hp_terminal )
			softkeys_off();

		if ( has_transmit )
			transmit_functions( OFF );

		retval = secure_display( lines, msgnumber );

		if ( has_transmit )
			transmit_functions( ON );

		Raw( ON );
	}


	return ( retval == 0 ? 1 : retval );	/* we did it boss! */
}


/*
 *  This next one is the 'pipe' file descriptor for writing to the 
 *  pager process... 
 */


FILE           *output_pipe;

int
display( lines, msgnum )

	int             lines, 
			msgnum;

{
	/*
	 *  Display specified number of lines from file mailfile.
	 *  Note: This routine MUST be placed at the first line 
	 *  of the input file! 
	 *  Returns the same as the routine above (namely zero or one)
	 */


	char            title1[SLEN], 
			title2[SLEN], 
			*p;
	char            from_buffer[VERY_LONG_STRING], 
			buffer[VERY_LONG_STRING];
#ifdef SIGWINCH
	struct winsize	w_before, w_after;
	void		(*wstat) ();
#endif


#ifdef ENCRYPTION_SUPPORTED
	int             crypted = 0, gotten_key = 0;	/* encryption */
#endif


	int             weed_header, 
			weeding_out = 0,	/* weeding    */
	                mail_sent,		/* misc use   */
	                form_letter = FALSE,	/* Form ltr?  */
	                form_letter_section = 0,/* section    */
	                padding = 0,		/* counter  */
	                builtin = FALSE,	/* our pager? */
	                val = 0;		/* return val */


	dprint( 4, (debugfile, "displaying %d lines from message %d using %s\n",
		   lines, msgnum, pager) );

	ClearScreen();

	pipe_abort = FALSE;

	builtin = ( first_word(pager, "builtin") || first_word(pager, "internal") );

	if ( form_letter = (header_table[msgnum - 1].status & FORM_LETTER) ) {
		if ( filter )
			form_letter_section = 1;	/* initialize to section
							 * 1 */
	}

	if ( builtin )
		start_builtin( lines );
	else {
		total_lines_to_display = lines;
		lines_displayed = 0;

#ifdef SIGWINCH
		ioctl(0, TIOCGWINSZ, &w_before);
#endif

		if ( (output_pipe = popen(pager, "w")) == NULL ) {
			error2( catgets(nl_fd,NL_SETN,4,"Can't create pipe to %s [%s]"), pager,
			        error_name(errno) );
			dprint( 1, (debugfile,
			        "\n*** Can't create pipe to %s - error %s ***\n\n",
				pager, error_name(errno)) );
			return ( 0 );
		}

#ifdef SIGWINCH
		wstat = signal(SIGWINCH, SIG_IGN);
#endif

		dprint( 4, (debugfile,
			"Opened a write-only pipe to pager %s \n", pager) );
	}

	if ( title_messages && filter ) {

		mail_sent = ( first_word(header_table[msgnum - 1].from, "To:") );

		tail_of( header_table[msgnum - 1].from, from_buffer, FALSE );

		sprintf( title1, "%s %d/%d %s %s%s",
		         header_table[msgnum - 1].status & DELETED ? "[deleted]" :
			 form_letter ? "Form" : "Message",
			 msgnum, message_count,
			 mail_sent ? "to" : "from", from_buffer,
			 strlen(from_buffer) > 26 ? "\n" : "" );

		sprintf( title2, " %s %s '%d at %s\n",
			 header_table[msgnum - 1].month,
			 header_table[msgnum - 1].day,
			 atoi(header_table[msgnum - 1].year),
			 header_table[msgnum - 1].time );

		/*
		 *  and now let's add some spaces between the two parts, please 
		 */

		if ( strlen(from_buffer) > 26 ) {
			if ( builtin )
				display_line( title1 );
			else
				fprintf( output_pipe, "%s", title1 );

			title1[0] = '\0';
			padding = COLUMNS - strlen( title2 ) - 1;

		} else
			padding = COLUMNS - strlen( title1 ) - strlen( title2 ) - 1;

		p = title1 + strlen( title1 );

		while ( padding-- > 0 )
			*p++ = ' ';

		*p = '\0';
		strcat( title1, title2 );

		if ( builtin )
			display_line( title1 );
		else
			fprintf( output_pipe, "%s", title1 );

		/*
		 *  if there's a subject, let's next output it, centered.  
		 */

		if ( strlen(header_table[current - 1].subject) > 0 &&
		    			matches_weedlist("Subject:") ) {
			/* This is a fix for SWFfc01322.  The subject line
			 * is longer than COLUMNS which ultimately causes
			 * a sigsegv.  The problem is that strlen is declared
			 * as a size_t which is unsigned.  When COLUMNS
			 * is less than the strlen value, padding gets 
			 * promoted to unsigned causing it to be a *VERY
			 * LARGE* value.  Simply casting the strlen to
			 * an int repairs the problem.
			 */
			padding = (COLUMNS - (int)strlen(header_table[current - 1].subject)) / 2;
			p = buffer;

			while ( padding-- > 0 )
				*p++ = ' ';

			*p = '\0';
			strcat( buffer, header_table[current - 1].subject );
			strcat( buffer, "\n" );

			if ( builtin )
				display_line( buffer );
			else
				fprintf( output_pipe, "%s", buffer );
		}

		/*
		 *  was this message address to us?  if not, then to whom? 
		 */

		if ( !mail_sent && matches_weedlist("To:") && filter &&
		     !equal(header_table[current - 1].to, username)  &&
		     strlen(header_table[current - 1].to) > 0 ) {
			if ( strlen(header_table[current - 1].to) > 60 )
				sprintf( buffer, catgets(nl_fd,NL_SETN,5,"%s(message addressed to %.60s)\n"),
					 strlen(header_table[current - 1].subject) > 0 ? 
					 "\n" : "", header_table[current - 1].to );
			else
				sprintf( buffer, catgets(nl_fd,NL_SETN,6,"%s(message addressed to %s)\n"),
					 strlen(header_table[current - 1].subject) > 0 ? 
					 "\n" : "", header_table[current - 1].to );

			if ( builtin )
				display_line( buffer );
			else
				fprintf( output_pipe, "%s", buffer );
		}

	 /*
	  *   The test above is: if we didn't originally send the mail
      	  *   (e.g. we're not reading "mail.sent") AND the user is currently
      	  *   weeding out the "To:" line (otherwise they'll get it twice!)
      	  *   AND the user is actually weeding out headers AND the message 
      	  *   wasn't addressed to the user AND the 'to' address is non-zero 
      	  *   (consider what happens when the message doesn't HAVE a "To:" 
      	  *   line...the value is NULL but it doesn't match the username 
      	  *   either.  We don't want to display something ugly like 
      	  *   "(message addressed to )" which will just clutter up the 
      	  *   screen!).
      	  */

	 /*   one more friendly thing - output a line indicating what sort
      	  *   of status the message has (e.g. Urgent etc).  Mostly added
      	  *   for X.400 support, this is nonetheless generally useful to
      	  *   include...
      	  */

		buffer[0] = '\0';

	 /*
	  * we want to flag Urgent, Confidential, Private and Expired
	  * tags 
	  */

		if ( header_table[current - 1].status & PRIVATE )
			strcpy( buffer, catgets(nl_fd,NL_SETN,7,"\n(** This message is tagged Private") );
		else if ( header_table[current - 1].status & CONFIDENTIAL )
			strcpy( buffer, catgets(nl_fd,NL_SETN,8,"\n(** This message is tagged Company Confidential") );

		if ( header_table[current - 1].status & URGENT ) {
			if ( buffer[0] == '\0' )
				strcpy( buffer, catgets(nl_fd,NL_SETN,9,"\n(** This message is tagged Urgent") );
			else if ( header_table[current - 1].status & EXPIRED )
				strcat( buffer, catgets(nl_fd,NL_SETN,10,", Urgent") );
			else
				strcat( buffer, catgets(nl_fd,NL_SETN,11," and Urgent") );
		}

		if ( header_table[current - 1].status & EXPIRED ) {
			if ( buffer[0] == '\0' )
				strcpy( buffer, catgets(nl_fd,NL_SETN,12,"\n(** This message has Expired") );
			else
				strcat( buffer, catgets(nl_fd,NL_SETN,13,", and has Expired") );
		}

		if ( buffer[0] != '\0' ) {
			strcat( buffer, " **)\n" );
			if ( builtin )
				display_line( buffer );
			else
				fprintf( output_pipe, buffer );
		}

		if ( builtin ) {		/* this is for a one-line blank    */
			buffer[0] = '\n';
			buffer[1] = '\0';
			display_line( buffer );	/* separator between the
						 * title   */
		} else		/* stuff and the actual message  */
			fprintf( output_pipe, "\n" );	/* we're trying to
							 * display       */

	}

	weed_header = filter;		/* allow us to change it after header */

	while ( lines > 0 && pipe_abort == FALSE ) {

		if ( fgets(buffer, VERY_LONG_STRING, mailfile) == NULL ) {
			if ( lines_displayed == 0 ) {

				dprint( 1, (debugfile,
					   "\n\n** Out of Sync!!  EOF with nothing read (display) **\n") );
				dprint( 1, (debugfile,
					   "** closing and reopening mailfile... **\n\n") );

				if ( !builtin )
#ifdef SIGWINCH
						{
#endif
					pclose( output_pipe );	
#ifdef SIGWINCH
					ioctl(0, TIOCGWINSZ, &w_after);
					if ((w_before.ws_row != w_after.ws_row) ||
							(w_before.ws_col != w_after.ws_col))
						setsize(w_after.ws_row, w_after.ws_col);
					signal(SIGWINCH, wstat);
				}
#endif

				if ( mailfile != NULL )
					fclose( mailfile );

				if ( (mailfile = fopen(infile, "r")) == NULL ) {
					error( catgets(nl_fd,NL_SETN,14,"Sync error: can't re-open mailbox!!") );
					show_mailfile_stats();
					emergency_exit();
				}

				return ( show_message(lines,
					    header_table[msgnum - 1].offset,
						     msgnum) );
			}

			if ( !builtin )
#ifdef SIGWINCH
					{
#endif
				pclose( output_pipe );
#ifdef SIGWINCH
				ioctl(0, TIOCGWINSZ, &w_after);
				if ((w_before.ws_row != w_after.ws_row) ||
					(w_before.ws_col != w_after.ws_col))
					setsize(w_after.ws_row, w_after.ws_col);
				signal(SIGWINCH, wstat);
			}
#endif

			if ( lines == 0 && pipe_abort == FALSE ) {    /* displayed it all */
				return ( return_to_elm() );
			}
		}

		if ( strlen(buffer) > 0 )
			no_ret( buffer );

		if ( strlen(buffer) == 0 ) {
			weed_header = 0;		/* past header! */
			weeding_out = 0;
		}

		if ( form_letter && weed_header );

			/*
			 * skip it.  NEVER display random headers in forms! 
			 */

		else if ( weed_header && matches_weedlist(buffer) )
			weeding_out = 1;	/* We don't want to see this! */


#ifdef ENCRYPTION_SUPPORTED
		else if ( buffer[0] == '[' ) {
			if ( first_word(buffer, START_ENCODE) ) {
				val = show_line( buffer, builtin );
				crypted = TRUE;
			} else if ( first_word(buffer, END_ENCODE) ) {
				crypted = FALSE;
				val = show_line( buffer, builtin );
			} else if ( crypted ) {
				encode( buffer );
				val = show_line( buffer, builtin );
			} else
				val = show_line( buffer, builtin );

		} else if ( crypted ) {
			if ( !gotten_key++ )
				getkey( OFF );
			encode( buffer );
			val = show_line( buffer, builtin );
		}
#endif


		else if ( weeding_out ) {
			weeding_out = ( whitespace(buffer[0]) );	/* 'n' line weed */
			if ( !weeding_out )				/* just turned on!*/
				val = show_line( buffer, builtin );

		} else if ( form_letter && first_word(buffer, "***") && filter ) {
			strcpy( buffer,
			       "\n------------------------------------------------------------------------------\n" );
			val = show_line( buffer, builtin ); 		/* hide '***' */
			form_letter_section++;

		} else if ( form_letter_section == 1 || form_letter_section == 3 );

			/*
			 * skip this stuff - we can't deal with it... 
			 */
			 
		else
			val = show_line( buffer, builtin );

		if ( val != 0 ) {		/* let's get back to the top level ... */
			if ( !builtin )
#ifdef SIGWINCH
					{
#endif
				pclose( output_pipe );
#ifdef SIGWINCH
				ioctl(0, TIOCGWINSZ, &w_after);
				if ((w_before.ws_row != w_after.ws_row) ||
					(w_before.ws_col != w_after.ws_col))
					setsize(w_after.ws_row, w_after.ws_col);
				signal(SIGWINCH, wstat);
			}
#endif
			return ( val );
		}

		lines_displayed++;
		lines = total_lines_to_display - lines_displayed;

	}

	if ( !builtin )
#ifdef SIGWINCH
			{
#endif
                pclose( output_pipe );
#ifdef SIGWINCH
		ioctl(0, TIOCGWINSZ, &w_after);
		if ((w_before.ws_row != w_after.ws_row) ||
				(w_before.ws_col != w_after.ws_col))
			setsize(w_after.ws_row, w_after.ws_col);
		signal(SIGWINCH, wstat);
	}
#endif

	if ( lines == 0 && pipe_abort == FALSE ) {		/* displayed it all! */
		return ( return_to_elm() );
	}

	return ( val );
}


int
show_line( buffer, builtin )

	char           *buffer;
	int             builtin;

{
	/*
	 *  Hands the given line to the output pipe.  'builtin' is true if
	 *  we're using the builtin pager.  We will return 'val' as the
         *  intermediate value from the builtin pager if the user chooses
	 *  to do something else at the end-of-page prompt! 
	 */


	register int    val;


	if ( builtin ) {

		if ( printable_chars(buffer) != COLUMNS )
			strcat( buffer, "\n" ); 	/* for auto_wrapping */

		if ( (val = display_line(buffer)) > 1 ) {
			return ( val );
		} else
			pipe_abort = val;

	} else {
		errno = 0;
		fprintf( output_pipe, "%s\n", buffer );

#ifdef DEBUG
		if ( errno != 0 )
			dprint( 1, (debugfile, "\terror %s hit!\n", error_name(errno)) );
#endif
	}

	return ( 0 );
}


int
secure_display( lines, msgnumber )

	int             lines, 
			msgnumber;

{
	/*
	 *  This is the cheap way to implement secure pipes - spawn a
	 *  child process running under the old userid, then open the
	 *  pager and feed the message to it.  When the subprocess ends
	 *  (the routine returns) simply return.  Simple and effective.
	 *  (too bad it's this much of a hassle to implement secure
	 *  pipes, though - I can imagine it being a constant problem!)
	 */


	int             pid, 
			w;
	int             status;
#ifdef SIGWINCH
	struct winsize	w_before, w_after;
#endif
	void		(*istat) (),
#ifdef SIGWINCH
			(*wstat) (),
#endif
			(*qstat) ()

#ifdef SIGSTOP
			,(*tstat) ()
#endif

			;


#ifdef SIGSTOP
	tstat = signal( SIGTSTP, SIG_IGN );
#endif

#ifdef SIGWINCH
	/* get the window size before forking the child */
	ioctl(0, TIOCGWINSZ, &w_before);
#endif

	/*
	 * we must invalidate the read buffer for the mail file because
	 * the child may read the file, which would otherwise leave the
	 * parent and the child out of sync on the stream.
	 */
	 fflush(mailfile);

#ifdef NO_VM				/* machine without virtual memory! */
	if ( (pid = fork()) == 0 ) {
#else
	if ( (pid = vfork()) == 0 ) {
#endif

		setgid( groupid );	/* and group id		    */
		setuid( userid );	/* back to the normal user! */

		_exit( display( lines, msgnumber ) );
	}

	istat = signal( SIGINT, SIG_IGN );
	qstat = signal( SIGQUIT, SIG_IGN );
#ifdef SIGWINCH
	wstat = signal( SIGWINCH, SIG_IGN );
#endif

	while ( (w = wait(&status)) != pid && w != -1 );

#ifdef SIGWINCH
	/* if the window sized changed while we were waiting, lets update */

	ioctl(0, TIOCGWINSZ, &w_after);
	if ((w_before.ws_row != w_after.ws_row) ||
			(w_before.ws_col != w_after.ws_col))
		setsize(w_after.ws_row, w_after.ws_col);
#endif

	signal( SIGINT, istat );
	signal( SIGQUIT, qstat );
#ifdef SIGWINCH
	signal( SIGWINCH, wstat );
#endif

#ifdef SIGSTOP
	signal( SIGTSTP, tstat );
#endif

	return ( 0 );
}


int
return_to_elm()

{
	/*
	 *  A central location for all the prompts etc 
	 */


	int             val;


	MoveCursor( LINES, 0 );

	StartInverse();
	Write_to_screen( catgets(nl_fd,NL_SETN,15," Please press <return> to return to Main : "), 0, NULL, NULL, NULL );
	EndInverse();

	CleartoEOS();

#ifdef SIGWINCH
	while((val = (char)tolower(ReadCh())) == (char)NO_OP_COMMAND)
		;
#else
	val = (char)tolower(ReadCh());
#endif

	ioctl( 0, TCFLSH, 0 ); /* flush any extranious characters typed before
				* the return was pressed */
	return ( val );
}
