/**			pattern.c			**/

/*
 *  @(#) $Revision: 72.2 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *     General pattern matching for the ELM mailer.     
 */

#include "headers.h"


#ifdef NLS
# define NL_SETN  30
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


static char     pattern[SLEN] = { "" };
static char     alt_pattern[SLEN] = { "" };

int
meta_match( function )

	int             function;

{
	/*
	 *  Perform specific function based on whether an entered string 
	 *  matches either the From or Subject lines.. 
	 */


	register int    i, 
			len,
			ret,
			tagged = 0, 
			count = 0;
	static char     meta_pattern[SLEN];


	PutLine1( LINES - 3, (int)strlen(catgets(nl_fd,NL_SETN,1,"Command: ")),
		 catgets(nl_fd,NL_SETN,2,"%s messages that match pattern..."),
		 function == TAGGED ? "Tag" : function == DELETED ? "Delete" : "Undelete" );

	if ( function == TAGGED ) {		/* are messages already tagged??? */

		for ( i = 0; i < message_count; i++ )
			if ( ison(header_table[i].status, TAGGED) )
				tagged++;

		if ( tagged ) {
			if ( tagged > 2 )
				PutLine0( LINES - 2, 0, catgets(nl_fd,NL_SETN,3,"Some messages are already tagged") );
			else
				PutLine0( LINES - 2, 0, catgets(nl_fd,NL_SETN,4,"A message is already tagged") );

			Write_to_screen( catgets(nl_fd,NL_SETN,5,"- Remove tag(s)? y%c"), 2, BACKSPACE, NULL, NULL );

			if ( hp_terminal )
				define_softkeys( YESNO );

			if ( (ret = want_to((char *) NULL, 'y', TRUE)) == 'y' ) {/* remove tags...*/

				for ( i = 0; i < message_count; i++ ) {
					clearit( header_table[i].status, TAGGED );
					show_new_status( i );
				}

			} else if ( ret == eof_char ) {

				if ( hp_terminal )
					define_softkeys( MAIN );
				ClearLine( LINES - 2 );
				return(0);
			}
		}
	}

	PutLine0( LINES - 2, 0, catgets(nl_fd,NL_SETN,6,"Enter pattern: ") );
	CleartoEOLN();

	if ( hp_terminal )
		define_softkeys( CANCEL );

	meta_pattern[0] = '\0';
	if ( optionally_enter( meta_pattern, LINES - 2, 
		  	  (int)strlen(catgets(nl_fd,NL_SETN,6,"Enter pattern: ")), FALSE )) {

		if ( hp_terminal )
			define_softkeys( MAIN );

		ClearLine( LINES - 2 );

		meta_pattern[0] = '\0';

		return( 0 );

	} else if ( strlen(meta_pattern) == 0 ) {

		if ( hp_terminal )
			define_softkeys( MAIN );

		ClearLine( LINES - 2 );
		return ( 0 );
	}

	strcpy( meta_pattern, shift_lower(meta_pattern) );	/* lowercase it */

	for ( i = 0; i < message_count; i++ ) {
		if ( from_matches(i, meta_pattern) ) {

			if ( (selected && header_table[i].status & VISIBLE) || !selected ) {
				if ( function == UNDELETE )
					clearit( header_table[i].status, DELETED );
				else
					setit( header_table[i].status, function );
				show_new_status( i );
				count++;
			}

		} else if ( subject_matches(i, meta_pattern) ) {

			if ( (selected && header_table[i].status & VISIBLE) || !selected ) {
				if ( function == UNDELETE )
					clearit( header_table[i].status, DELETED );
				else
					setit( header_table[i].status, function );
				show_new_status( i );
				count++;
			}

		}
	}

	ClearLine( LINES - 2 );			/* remove "pattern: " prompt */

	if ( count > 0 ) {
		error2( catgets(nl_fd,NL_SETN,7,"%s %d messsage(s)"),
		        function == TAGGED ? catgets(nl_fd,NL_SETN,8,"tagged") :
		        function == DELETED ? catgets(nl_fd,NL_SETN,9,"marked for deletion") 
						: catgets(nl_fd,NL_SETN,10,"undeleted"), count );

		len = strlen( meta_pattern );

		if ( len >= COLUMNS - 15 )	/* for next search */
			meta_pattern[COLUMNS-15-2] = '\0';

	} else {
		error1( catgets(nl_fd,NL_SETN,11,"no matches - no messages %s"),
		        function == TAGGED ? catgets(nl_fd,NL_SETN,8,"tagged") :
		 	function == DELETED ? catgets(nl_fd,NL_SETN,9,"marked for deletion") 
						: catgets(nl_fd,NL_SETN,10,"undeleted") );

		meta_pattern[0] = '\0';
	}

	if ( hp_terminal )
		define_softkeys( MAIN );

	return ( 0 );
}


int
pattern_match()

{
	/*
	 *  Get a pattern from the user and try to match it with the
	 *  from/subject lines being displayed.  If matched (ignoring
	 *  case), move current message pointer to that message, if
	 *  not, error and return ZERO.
	 */


	register int    i,
			len;
	int		rlt;


	PutLine0( LINES - 3, 40, catgets(nl_fd,NL_SETN,12,"/ = match anywhere in messages") );

	PutLine0( LINES - 2, 0, catgets(nl_fd,NL_SETN,13,"Match Pattern: ") );

	if ( hp_terminal )
		define_softkeys( CANCEL );

	if ( (rlt = pattern_enter(pattern, alt_pattern, LINES - 2, 15,
			  catgets(nl_fd,NL_SETN,14,"Match Pattern (in entire mailbox):"))) == 1 ) {

		if ( strlen(alt_pattern) > 0 ) {
			strcpy( alt_pattern, shift_lower(alt_pattern) );
			rlt = match_in_message(alt_pattern);
		} else
			rlt = 1;
		
		if ( rlt == 1 ) {
			len = strlen( alt_pattern );

			if ( len >= COLUMNS - 34 ) /* for next search */
				alt_pattern[COLUMNS-34-2] = '\0';
		} else
			alt_pattern[0] ='\0';

		if ( hp_terminal )
			define_softkeys( MAIN );

		return( rlt );

	} else if ( rlt == -1 && strlen(pattern) == 0 ) {
		
		if ( hp_terminal )
			define_softkeys( MAIN );

		return ( 1 );
	} else
		strcpy( pattern, shift_lower(pattern) );

	for ( i = current; i < message_count; i++ ) {

		if ( from_matches(i, pattern) ) {
			if ( !selected || (selected && header_table[i].status & VISIBLE) ) {

				if ( current == i + 1 )
					error( catgets(nl_fd,NL_SETN,15,"only occurrence is in the current message") );
				else
					current = i + 1;
		
				/* for next search */

				len = strlen( pattern );

				if ( len >= COLUMNS - 15 )
					pattern[COLUMNS-15-2] = '\0';

				if ( hp_terminal )
					define_softkeys( MAIN );

				return ( 1 );

			}

		} else if ( subject_matches(i, pattern) ) {
			if ( !selected || (selected && header_table[i].status & VISIBLE) ) {
				if ( current == i + 1 )
					error( catgets(nl_fd,NL_SETN,15,"only occurrence is in the current message") );
				else
					current = i + 1;
		
				len = strlen( pattern );

				if ( len >= COLUMNS - 15 ) 
					pattern[COLUMNS-15-2] = '\0';

				if ( hp_terminal )
					define_softkeys( MAIN );

				return ( 1 );
			}
		}

	}

	/*
	 *  now let's wrap around the end and search up thru the current
	 *  message ...
	 */

	for ( i = 0; i < current; i++ ) {

		if ( from_matches(i, pattern) ) {
			if ( !selected || (selected && header_table[i].status & VISIBLE) ) {
				if ( current == i + 1 )
					error( catgets(nl_fd,NL_SETN,15,"only occurrence is in the current message") );
				else
					current = i + 1;
		
				len = strlen( pattern );

				if ( len >= COLUMNS - 15 )
					pattern[COLUMNS-15-2] = '\0';

				if ( hp_terminal )
					define_softkeys( MAIN );

				return ( 1 );
			}
		} else if ( subject_matches(i, pattern) ) {
			if ( !selected || (selected && header_table[i].status & VISIBLE) ) {
				if ( current == i + 1 )
					error( catgets(nl_fd,NL_SETN,15,"only occurrence is in the current message") );
				else
					current = i + 1;
		
				len = strlen( pattern );

				if ( len >= COLUMNS - 15 )
					pattern[COLUMNS-15-2] = '\0';

				if ( hp_terminal )
					define_softkeys( MAIN );

				return ( 1 );
			}
		}

	}

	pattern[0] = '\0';
		
	if ( hp_terminal )
		define_softkeys( MAIN );

	return ( 0 );
}


int
from_matches( message_number, pat )

	int             message_number;
	char           *pat;

{
	/*
	 *  Returns true iff the pattern occurs in it's entirety
	 *  in the from line of the indicated message 
	 */

	return ( in_string(shift_lower(header_table[message_number].from), pat) );
}


int
subject_matches( message_number, pat )

	int             message_number;
	char           *pat;

{
	/*
	 *  Returns true iff the pattern occurs in it's entirety
	 *  in the subject line of the indicated message 
	 */

	return ( in_string(shift_lower(header_table[message_number].subject), pat) );
}


int
match_in_message( pat )

	char           *pat;

{
	/*
	 *  Match a string INSIDE a message...starting at the current 
	 *  message read each line and try to find the pattern.  As
	 *  soon as we do, set current and leave!   This routine also
	 *  does wrap-searching, where it'll hit the last message in
	 *  the mailbox, then start from message #1 up to the current
	 *  message in an attempt to find what we're asking for.
	 *
	 *  Returns 1 if found, 0 if not
	 */


	char            buffer[VERY_LONG_STRING];
	int             message_number, 
			last_message_number, 
			iterations = 0, 
			lines, 
			line;


	message_number = current;
	last_message_number = message_count;

	error( catgets(nl_fd,NL_SETN,16,"searching mailbox for pattern...") );

top_of_loop:

	while ( message_number < last_message_number ) {

		if ( fseek(mailfile, header_table[message_number].offset, 0L) == -1 ) {

			dprint( 1, (debugfile,
				"Error: seek %ld bytes into file failed. errno %d (%s)\n",
				 header_table[message_number].offset, errno,
				 "match_in_message") );
			error2( catgets(nl_fd,NL_SETN,17,"ELM [match] failed looking %ld bytes into file (%s)"),
			        header_table[message_number].offset, error_name(errno) );

			return ( 1 );			/* fake it out to avoid replacing
					 	 	 * error message */
		}

		line = 0;
		lines = header_table[message_number].lines;

		while ( fgets(buffer, VERY_LONG_STRING, mailfile) != NULL && line < lines ) {

			line++;

			if ( in_string(shift_lower(buffer), pat) ) {

				if ( current == message_number + 1 )
					error( catgets(nl_fd,NL_SETN,15,"only occurrence is in the current message") );
				else {
					current = message_number + 1;
					clear_error();
				}

				return ( 1 );
			}
		}

		/*
		 *  now we've passed the end of THIS message...increment and 
      	         *  continue the search with the next message! 
		 */

		message_number++;
	}

	/*
	 *  if we get here, it means we've not found it from the message
	 *  we're on +1 through the end of the mailbox.  Now we'll try
	 *  searching from the beginning up to the current message...
	 */

	if ( iterations++ )
		return ( 0 );		/* we've already tried that trick - BAIL! */

	message_number = 0;
	last_message_number = current;

	goto top_of_loop;
}
