/** 			mailmsg1.c			**/

/*
 *  @(#) $Revision: 70.4 $
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

#include "headers.h"


#ifdef NLS
# define NL_SETN  25
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


/*
 *  strings defined for the hdrconfg routines 
 */

char            subject[SLEN], 
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


#ifdef ENABLE_NAMESERVER
extern int      screen_changed;
#endif


extern int	num_char;

int
send_msg( given_to, given_cc, given_subject, edit_message, form_letter, replying )

	char            *given_to, 
			*given_cc, 
			*given_subject;
	int             edit_message, 
			form_letter, 
			replying;

{
	/*
	 *  Prompt for fields and then call mail() to send the specified
	 *  message.  If 'edit_message' is true then don't allow the
         *  message to be edited. 'form_letter' can be "YES" "NO" or "MAYBE".
	 *  if YES, then add the header.  If MAYBE, then add the M)ake form
	 *  option to the last question (see mailsg2.c) etc. etc. 
	 *  if (replying) then add an In-Reply-To: header...
	 *  Return value 0 means no needs of redrawing screen and 1 means
	 *  it needs redrawing.
	 */


	int             copy_msg = FALSE, 
			is_a_response = FALSE;

	/*
	 * First: zero all current global message strings 
	 */

	to[0] = cc[0] = reply_to[0] = expires[0] = '\0';

	action[0] = priority[0] = '\0';


#ifdef ALLOW_BCC
	bcc[0] = expanded_bcc[0] = '\0';
#endif


	in_reply_to[0] = expanded_to[0] = expanded_cc[0] = '\0';
	user_defined_header[0] = '\0';

	strcpy( subject, given_subject );		/* copy given subject */
	strcpy( to, given_to );				/* copy given to:     */
	strcpy( cc, given_cc );				/* and so on..       */

	/*
	 ******* And now the real stuff! ******* 
	 */

	copy_msg = copy_the_msg( &is_a_response );	/* copy msg into edit
							 * buffer? */
	if ( copy_msg == CANCEL )
		return ( 0 );

	if ( hp_terminal )
		define_softkeys( CANCEL );

	if ( get_to(to, expanded_to) == 0 )		/* get the To: address and
							 * expand */

#ifdef ENABLE_NAMESERVER
		return ( screen_changed );
#else
		return ( 0 );
#endif


	/*
	 *  are we by any chance just checking the addresses? 
	 */

	if ( check_only ) {
		printf( "Expands to: %s\n", format_long(expanded_to, 12) );
		(void) putchar( '\r' );			/* don't ask... */
		leave( 0 );
	}

	/*
	 *  if we're batchmailing, let's send it and GET OUTTA HERE! 
	 */

	if ( mail_only && strlen(batch_subject) > 0 ) {
		strcpy( subject, batch_subject );	/* get the batch subject */
		return ( mail(FALSE, FALSE, TRUE, form_letter) );
	}

	display_to( expanded_to );		/* display the To: field on screen... */

	dprint( 3, (debugfile, "\nMailing to %s\n", expanded_to) );

	if ( get_subject(subject) == 0 )	/* get the Subject: field */


#ifdef ENABLE_NAMESERVER
		return ( screen_changed );
#else
		return ( 0 );
#endif


	dprint( 4, (debugfile, "Subject is %s\n", subject) );

	if ( prompt_for_cc || (replying == 2  &&  strlen(cc) != 0) ) {
		if ( get_copies(cc, expanded_to, expanded_cc) == 0 )


#ifdef ENABLE_NAMESERVER
			return ( screen_changed );
#else
			return ( 0 );
#endif


#ifdef DEBUG
		if ( strlen(cc) > 0 )
			dprint( 4, (debugfile, "Copies to %s\n", expanded_cc) );
#endif
	}


#ifdef ALLOW_BCC
	if ( prompt_for_bcc ) {
		if ( get_blind_copies(bcc, expanded_bcc, copy_msg) == 0 )

#ifdef ENABLE_NAMESERVER
			return ( screen_changed );
#else
			return ( 0 );
#endif

#ifdef DEBUG
		if ( strlen(bcc) > 0 )
			dprint( 4, (debugfile, "Blind Copies to %s\n", expanded_bcc) );
#endif
	}
#endif


	if ( mail_only )			/* indicate next step... */
		printf( "\n\r" );
	else
		MoveCursor( LINES, 0 );		/* so you know you've hit <return> ! */

	/*
	 *  generate the In-Reply-To: header...
	 */

	if ( is_a_response && (replying != 0) )
		generate_reply_to( current - 1 );

	/*
	 * and mail that puppy outta here! 
	 */

	return( mail( copy_msg, edit_message, FALSE, form_letter ) );

}


int 
get_to( to_field, address )

	char            *to_field, 
			*address;

{
	/*
	 *  prompt for the "To:" field, expanding into address if possible.
	 *  This routine returns ZERO if errored, or non-zero if okay 
	 */


#ifdef ENABLE_NAMESERVER
	screen_changed = 0;			/* to keep track of nameserver stuff */
#endif


	if ( strlen(to_field) == 0 ) {
	
		if ( mail_only ){
			printf( user_level < 2 ? "Send the message to: " : "To: " );
			fflush( stdout );
		} else {
			if ( user_level < 1 ) {
				PutLine0( LINES - 2, 0,catgets(nl_fd,NL_SETN,9, "Send the message to: " ));
				num_char = strlen(catgets(nl_fd,NL_SETN,9, "Send the message to: " )) ;
				}
			else {
				PutLine0( LINES - 2, 0,catgets(nl_fd,NL_SETN,10, "To: " ));
				num_char = strlen(catgets(nl_fd,NL_SETN,10, "To: "));
				}
			
			CleartoEOS();
		}		

		if ( optionally_enter(to_field, LINES - 2, num_char, FALSE) ){
			if ( !mail_only ){
				MoveCursor( LINES - 2, 0 );
				CleartoEOS();
				error( catgets(nl_fd,NL_SETN,1,"mail not sent") );
			} else 
				printf( catgets(nl_fd,NL_SETN,2,"\r\n\r\nMail Cancelled!\n\r") );
			return( 0 );
		}
		
		if ( strlen(to_field) == 0 ) {
			if ( !mail_only ){
				MoveCursor( LINES - 2, 0 );
				CleartoEOS();
				error( catgets(nl_fd,NL_SETN,1,"mail not sent") );
			} else 
				printf( catgets(nl_fd,NL_SETN,2,"\r\n\r\nMail Cancelled!\n\r") );
			return( 0 );
		}


#ifdef ENABLE_NAMESERVER
		MoveCursor( LINES - 1, 0 );	/* some feedback on user
						 * action here in case the
						 * nameserver is slow...  */
#endif


		(void) build_address( to_field, address );

	} else if ( mail_only )

		(void) build_address( to_field, address );
	else
		strcpy( address, to_field );

	if ( strlen(address) == 0 ) {		/* bad address!  Removed!! */
		if ( !mail_only )
			ClearLine( LINES - 2 );
		return ( 0 );
	}

	return ( 1 );				/* everything is okay... */
}


int 
get_subject( subject_field )

	char           *subject_field;

{
	/*
	 *  get the subject and return non-zero if all okay... 
	 */

	if ( mail_only ) {
		printf( "Subject: " );
		fflush( stdout );
	} else if ( user_level == 0 ) {
		PutLine0( LINES - 2, 0,catgets(nl_fd,NL_SETN,11, "Subject of message: " ));
		num_char = strlen(catgets(nl_fd,NL_SETN,11, "Subject of message: " ));
	} else {
		PutLine0( LINES - 2, 0,catgets(nl_fd,NL_SETN,12, "Subject: " ));
		num_char = strlen(catgets(nl_fd,NL_SETN,12, "Subject: " ));
		}

	CleartoEOS();

	if ( optionally_enter(subject_field, LINES - 2, num_char, TRUE) ) {

		/*
		 *  User hit the BREAK key or ctrl-'D' to cancel
		 */

		if ( !mail_only ){
			MoveCursor( LINES - 2, 0 );
			CleartoEOS();
			error( catgets(nl_fd,NL_SETN,1,"mail not sent") );
		} else 
			printf( catgets(nl_fd,NL_SETN,2,"\r\n\r\nMail Cancelled!\n\r") );

		return( 0 );
	}

	if ( strlen(subject_field) == 0 ) {		/* zero length subject?? */
		if ( mail_only )
			printf( catgets(nl_fd,NL_SETN,3,"\n\rNo subject - Continue with message? (y/n) n%c"),
			        BACKSPACE );
		else
			PutLine1( LINES - 2, 0, 
				catgets(nl_fd,NL_SETN,4,"No subject - Continue with message? (y/n) n%c"),
				BACKSPACE );

		if ( yesno('n', TRUE) == 'n' ) {	/* user says no! */
			if ( mail_only ) {
				printf( catgets(nl_fd,NL_SETN,2,"\r\n\r\nMail Cancelled!\n\r") );
			} else {
				MoveCursor( LINES - 2, 0 );
				CleartoEOS();
				error( catgets(nl_fd,NL_SETN,1,"mail not sent") );
			}
			return ( 0 );

		} else {
			if ( mail_only )
				printf( "\r\n" );
			else {
				PutLine0( LINES - 2, 0,catgets(nl_fd,NL_SETN,13, "Subject: <none>" ));
				CleartoEOS();
			}
		}
	}

	return ( 1 );				/** everything is cruising along okay **/
}


int 
get_copies( cc_field, address, addressII )

	char            *cc_field, 
			*address, 
			*addressII;

{
	/*
	 *  Get the list of people that should be cc'd, returning ZERO if
	 *  any problems arise.  Address and AddressII are for expanding
	 *  the aliases out after entry! 
	 *  If 'bounceback' is nonzero, add a cc to ourselves via the remote
	 *  site, but only if hops to machine are > bounceback threshold.
	 *  If copy-message, that means that we're going to have to invoke
	 *  a screen editor, so we'll need to delay after displaying the
	 *  possibly rewritten Cc: line...
	 */


	int		ret_val,	/* return value of build_address */
			has_given;


	if ( mail_only )
		printf( "\n\rCopies To: " );
	else
		PutLine0( LINES - 1, 0,catgets(nl_fd,NL_SETN,14, "Copies To: " ));

	num_char = strlen(catgets(nl_fd,NL_SETN,14, "Copies To: " ));

	fflush( stdout );
	CleartoEOS();

	if ( (has_given = strlen( cc_field ))== 0 ) {
		if ( optionally_enter(cc_field, LINES - 1, num_char, FALSE) ) {
			if ( mail_only ) {
				printf( catgets(nl_fd,NL_SETN,5,"\n\r\n\rMail not sent!\n\r") );
			} else {
				MoveCursor( LINES - 2, 0 );
				CleartoEOS();
				error( catgets(nl_fd,NL_SETN,1,"mail not sent") );
			}
			return ( 0 );
		}

	}

	ret_val = build_address(cc_field, addressII);

	/*
	 *  The following test is that if the build_address routine had
	 *  reason to rewrite the entry given, then, if we're mailing only
	 *  print the new Cc line below the old one.  If we're not, then
	 *  assume we're in screen mode and replace the incorrect entry on
	 *  the line above where we are (e.g. where we originally prompted
	 *  for the Cc: field).
	 */

	if ( ret_val || has_given )

		if ( mail_only )
			printf( "\rCopies To: %s\r\n", format_long(addressII, 4) );
		else {
			PutLine1( LINES - 1, 11, "%s\r\n", addressII );
/* Remove following lines
			if ( (!equal(editor, "builtin") && !equal(editor, "none"))
			    || copy_message )
				(void) sleep( 1 );
 */
		}

	if ( strlen(address) + strlen(addressII) > VERY_LONG_STRING ) {
		dprint( 2, (debugfile,
			   "String length of \"To:\" + \"Cc\" too long! (get_copies)\n") );
	 	if (mail_only)
			printf("\rToo many people.  Copies ignored\r\n");
		else
			error( catgets(nl_fd,NL_SETN,6,"Too many people.  Copies ignored") );
		(void) sleep( 2 );
		cc_field[0] = '\0';
	}


#ifdef ENABLE_NAMESERVER
	strcpy( cc_field, addressII );			/* save expanded value */
#endif


	return ( 1 );					/* everything looks okay! */
}


#ifdef ALLOW_BCC

int 
get_blind_copies( bcc_field, address, copy_message )

	char            *bcc_field, 
			*address; 
	int             copy_message;
{
	/*
	 *  Get the list of people that should be bcc'd, returning ZERO if
	 *  any problems arise.  Address and AddressII are for expanding
	 *  the aliases out after entry! 
	 *  If copy-message, that means that we're going to have to invoke
	 *  a screen editor, so we'll need to delay after displaying the
	 *  possibly rewritten Bcc: line...
	 */


	if ( mail_only )
		printf( "\n\rBlind Copies To: " );
	else
		PutLine0( LINES, 0,catgets(nl_fd,NL_SETN,15, "Blind Copies To: " ));

	num_char = strlen(catgets(nl_fd,NL_SETN,15, "Blind Copies To: "));

	fflush( stdout );
	CleartoEOS();

	if ( optionally_enter(bcc_field, LINES, num_char, FALSE) ) {
		if ( mail_only ) {
			printf( catgets(nl_fd,NL_SETN,5,"\n\r\n\rMail not sent!\n\r") );
		} else {
			if ( strlen(expanded_cc) < COLUMNS - 11 ) {
				MoveCursor( LINES - 2, 0 );
				CleartoEOS();
			} else
				showscreen();
			error( catgets(nl_fd,NL_SETN,1,"mail not sent") );
		}
		return ( 0 );
	}

	/*
	 *  The following test is that if the build_address routine had
	 *  reason to rewrite the entry given, then, if we're mailing only
	 *  print the new Bcc line below the old one.  If we're not, then
	 *  assume we're in screen mode and replace the incorrect entry on
	 *  the line above where we are (e.g. where we originally prompted
	 *  for the Bcc: field).
	 */

	if ( build_address(bcc_field, address) )

		if ( mail_only )
			printf( "\rBlind Copies To: %s\r\n", format_long(address, 4) );
		else {
			PutLine1( LINES, 17, "%s", address );
			if ( (!equal(editor, "builtin") && !equal(editor, "none"))
			    || copy_message )
				(void) sleep( 2 );
		}

	if ( strlen(expanded_to) + strlen(expanded_cc) + strlen(address)
			> VERY_LONG_STRING ) {

		dprint( 2, (debugfile,
			   "String length of \"To:\" + \"Cc\" + \"Bcc\" too long! (get_blind_copies)\n") );
		if ( mail_only )
			printf("\rToo many people.  Blind Copies ignored\r\n");
		else
			error( catgets(nl_fd,NL_SETN,7,"Too many people.  Blind Copies ignored") );
		(void) sleep( 2 );
		bcc_field[0] = '\0';
	}


#ifdef ENABLE_NAMESERVER
	strcpy( bcc_field, address );			/* save expanded value */
#endif


	return ( 1 );					/* everything looks okay! */
}

#endif


int
copy_the_msg( is_a_response )

	int            *is_a_response;

{
	/*
	 *  Returns True iff the user wants to copy the message being
	 *  replied to into the edit buffer before invoking the editor! 
	 *  Sets "is_a_response" to true if message is a response...
	 */


	int             answer = FALSE;


	if ( strlen(to) > 0 && !mail_only ) {		/* predefined 'to' line! */
		if ( auto_copy )
			answer = TRUE;
		else{
			if ( hp_terminal )
				define_softkeys( YESNO );

			answer =  want_to(catgets(nl_fd,NL_SETN,8,"Copy message? (y/n) "), 'n', TRUE);
			if ( answer == 'y' )
				answer = TRUE;
			else if ( answer == 'n' )
				answer = FALSE;
			else if ( answer == eof_char )
				answer = CANCEL;
		}

		*is_a_response = TRUE;

	} else if ( strlen(subject) > 0 )		/* predefined 'subject' (Forward) */
		answer = TRUE;

	return ( answer );
}


int
display_to( address )

	char           *address;

{
	/*
	 *  Simple routine to display the "To:" line according to the
	 *  current configuration (etc) 			      
	 */


	register int    open_paren;


	if ( mail_only )
		printf( "To: %s\n\r", format_long(address, 3) );
	else {


#ifdef ENABLE_NAMESERVER
		if ( screen_changed )
			printf( "\n\r\n\r\n\r\n\r" );
#endif


		if ( names_only )
			if ( (open_paren = chloc(address, '(')) > 0 ) {
				if ( open_paren < chloc(address, ')') ) {
					output_abbreviated_to( address );
					return;
				}
			}


#ifdef ENABLE_NAMESERVER
		if ( screen_changed ) {
			if ( strlen(address) > COLUMNS - 5 )
				PutLine1( LINES - 3, 0,catgets(nl_fd,NL_SETN,16,"To: (%s)"),
				      tail_of_string(address, COLUMNS - 5) );
			else
				PutLine1( LINES - 3, 0,catgets(nl_fd,NL_SETN,17,"To: %s"), address );
		} else {
#endif


			if ( strlen(address) > 45 )
				PutLine1( LINES - 3, COLUMNS - 50,catgets(nl_fd,NL_SETN,16,"To: (%s)"),
					 tail_of_string(address, 40) );
			else if ( strlen(address) > 30 )
				PutLine1( LINES - 3, COLUMNS - 50,catgets(nl_fd,NL_SETN,17,"To: %s"), address );
			else
				PutLine1( LINES - 3, COLUMNS - 50,catgets(nl_fd,NL_SETN,18,"          To: %s"), address );

#ifdef ENABLE_NAMESERVER
		}
#endif


		CleartoEOLN();
	}
}


int
output_abbreviated_to( address )

	char           *address;

{
	/*
	 *  Output just the fields in parens, separated by commas if need
	 *  be, and up to COLUMNS-50 characters...This is only used if the
	 *  user is at level BEGINNER.
	 */


	char            newaddress[VERY_LONG_STRING];
	register int    indx, 
			max = VERY_LONG_STRING, 
			newindx = 0, 
			in_paren = 0;

	indx = 0;


#ifdef ENABLE_NAMESERVER
	if ( screen_changed )
		max = COLUMNS - 5;
#endif


	while ( newindx < max && indx < strlen(address) ) {
		if ( address[indx] == '(' )
			in_paren++;
		else if ( address[indx] == ')' ) {
			in_paren--;
			if ( !in_paren ) {
				newaddress[newindx++] = ',';
				newaddress[newindx++] = ' ';
			}
		}

		if ( in_paren && !(in_paren == 1 && address[indx] == '(') )
			newaddress[newindx++] = address[indx];

		indx++;
	}

	newaddress[newindx] = '\0';

	if ( strlen(newaddress) != 0 )
		newaddress[newindx-2] = '\0'; 	/* remove last ', ' */


#ifdef ENABLE_NAMESERVER
	if ( screen_changed ) {
		if ( strlen(newaddress) > COLUMNS - 10 )
			PutLine1( LINES - 3, 0,catgets(nl_fd,NL_SETN,16,"To: (%s)"),
				 tail_of_string(newaddress, COLUMNS - 10) );
		else
			PutLine1( LINES - 3, 0,catgets(nl_fd,NL_SETN,17,"To: %s"), newaddress );

		CleartoEOLN();
	} else {
#endif

		if ( strlen(newaddress) > 50 )
			PutLine1( LINES - 3, COLUMNS - 50,catgets(nl_fd,NL_SETN,17,"To: %s"),
				 tail_of_string(newaddress, 40) );
		else {
			if ( strlen(newaddress) > 30 )
				PutLine1( LINES - 3, COLUMNS - 50,catgets(nl_fd,NL_SETN,17,"To: %s"), newaddress );
			else
				PutLine1( LINES - 3, COLUMNS - 50,catgets(nl_fd,NL_SETN,18,"          To: %s"),
				  	  newaddress );
			CleartoEOLN();
		}


#ifdef ENABLE_NAMESERVER
	}
#endif


	return;
}
