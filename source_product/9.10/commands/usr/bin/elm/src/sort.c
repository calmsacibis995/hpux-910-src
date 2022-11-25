/**			sort.c				**/

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
 *  Sort mailbox header table by the field specified in the global
 *  variable "sortby"...if we're sorting by something other than
 *  the default SENT_DATE, also put some sort of indicator on the
 *  screen.
 *
 */

#include "headers.h"


#ifdef NLS
# define NL_SETN   42
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


char		*sort_name(), 
		*skip_re();

static char	skipped_buf[SLEN];


int
sort_mailbox( entries, always_top, visible )

        int             always_top,
	                entries, 
			visible;

{
	/*
	 *  Sort the header_table definitions... If 'visible', then
	 *  put the status lines etc 
	 */


	int             last_index = -1;
	int             compare_headers();	/* for sorting */


	dprint( 2, (debugfile, "\n** sorting mailbox by %s **\n\n",
		   sort_name(SHORT)) );

	if ( entries > 0 && !always_top )
		last_index = header_table[current - 1].index_number;

	if ( entries > 30 && visible )
		error1( catgets(nl_fd,NL_SETN,1,"sorting messages by %s"), sort_name(SHORT) );

	qsort( (void *)header_table, (size_t)entries, sizeof(struct header_rec),
	      compare_headers );

	if ( last_index > -1 )
		find_old_current( last_index );

	if ( always_top )
	        current = 1;

	get_page( current );

	clear_error();
}


int
compare_headers( first, second )

	struct header_rec 	*first, 
				*second;

{
	/*
	 *  compare two headers according to the sortby value.
 	 * 
	 *  Sent Date uses a routine to compare two dates,
	 *  Received date is keyed on the file offsets (think about it)
	 *  Sender uses the truncated from line, same as "build headers",
	 *  and size and subject are trivially obvious!!
	 *  (actually, subject has been modified to ignore any leading
	 *  patterns [rR][eE]*:[ \t] so that replies to messages are
	 *  sorted with the message (though a reply will always sort to
	 *  be 'greater' than the basenote)
	 */


	char            from1[VERY_LONG_STRING], 
			from2[VERY_LONG_STRING]; /* sorting buffers... */
	int             sign = 1;


	if ( sortby < 0 )
		sign = -1;

	switch ( abs(sortby) ) {

	case SENT_DATE:
		return ( sign * compare_dates(first, second) );

	case RECEIVED_DATE:
		return ( sign *
			compare_parsed_dates(first->received,
					     second->received) );

	case SENDER:
		tail_of( first->from, from1, TRUE );
		tail_of( second->from, from2, TRUE );
		return ( sign * strcmp(from1, from2) );

	case SIZE:
		return ( sign * (first->lines - second->lines) );

	case MAILBOX_ORDER:
		return ( sign *
			(first->index_number - second->index_number) );

	case SUBJECT:		/* need some extra work 'cause of STATIC
				 * buffers */
		strcpy( from1, skip_re(shift_lower(first->subject)) );
		return ( sign *
			strcmp(from1,
			       skip_re(shift_lower(second->subject))) );

	case STATUS:
		return ( sign * (first->status - second->status) );
	}

	return ( 0 );		/* never get this! */
}


char    
*sort_name( type )

	int             type;

{
	/*
	 *  return the name of the current sort option...
	 *  type can be "FULL", "SHORT" or "PAD"
	 */


	int             pad, 
			abr;


	pad = ( type == PAD );		
	abr = ( type == SHORT );		 /* same as the contents of elmrc file */

	if ( sortby < 0 ) {
		switch ( -sortby ) {
		case SENT_DATE:
			return ( 
				pad ? "reverse-sent    " :
				abr ? "reverse-sent" :
				"Reverse Date Mail Sent" );
		case RECEIVED_DATE:
			return ( 
				pad ? "reverse-received" :
				abr ? "reverse-received" :
				"Reverse Date Mail Rec'vd" );

		case MAILBOX_ORDER:
			return ( 
				pad ? "reverse-mailbox " :
				abr ? "reverse-mailbox" :
				"Reverse Mailbox Order" );

		case SENDER:
			return ( 
				pad ? "reverse-from    " :
				abr ? "reverse-from" :
				"Reverse Message Sender" );
		case SIZE:
			return ( 
				pad ? "reverse-lines   " :
				abr ? "reverse-lines" :
				"Reverse Lines in Message" );
		case SUBJECT:
			return ( 
				pad ? "reverse-subject " :
				abr ? "reverse-subject" :
				"Reverse Message Subject" );
		case STATUS:
			return ( 
				pad ? "reverse-status  " :
				abr ? "reverse-status" :
				"Reverse Message Status" );
		}
	} else {
		switch ( sortby ) {
		case SENT_DATE:
			return ( 
				pad ? "sent            " :
				abr ? "sent" :
				"Date Mail Sent" );
		case RECEIVED_DATE:
			return ( 
				pad ? "received        " :
				abr ? "received" :
				"Date Mail Sent" );
		case MAILBOX_ORDER:
			return ( 
				pad ? "mailbox         " :
				abr ? "mailbox" :
				"Mailbox Order" );
		case SENDER:
			return ( 
				pad ? "from            " :
				abr ? "from" :
				"Message Sender" );
		case SIZE:
			return ( 
				pad ? "lines           " :
				abr ? "lines" :
				"Lines in Message" );
		case SUBJECT:
			return ( 
				pad ? "subject         " :
				abr ? "subject" :
				"Message Subject" );
		case STATUS:
			return ( 
				pad ? "status          " :
				abr ? "status" :
				"Message Status" );
		}
	}

	return ( "*UNKNOWN-SORT-PARAMETER*" );
}


int
find_old_current( indx )

	int             indx;

{
	/*
	 *  Set current to the message that has "indx" as its
	 *  index number.  This is to track the current message
	 *  when we resync... 
	 */


	register int    i;


	dprint( 4, (debugfile, "find-old-current(%d)\n", indx) );

	for ( i = 0; i < message_count; i++ )
		if ( header_table[i].index_number == indx ) {
			current = i + 1;
			dprint( 4, (debugfile, "\tset current to %d!\n", current) );
			return;
		}

	dprint( 4, (debugfile,
		   "\tcouldn't find current index.  Current left as %d\n",
		   current) );
	return;			/* can't be found.  Leave it alone, then */
}


char
*skip_re( string )

	char           *string;

{
	/*
	 *  this routine returns the given string minus any sort of
	 *  "re:" prefix.  specifically, it looks for, and will
	 *  remove, any of the pattern:
	 *
	 *	( [Rr][Ee][^:]:[ ] ) *
	 *
	 *  If it doesn't find a ':' in the line it will return it
	 *  intact, just in case!
	 */


	register int    i = 0;


	while ( whitespace(string[i]) )
		i++;

	do {
		if ( string[i] == '\0' )
			return ( (char *) string );		/* forget it */

		if ( string[i] != 'r' || string[i + 1] != 'e' )
			return ( (char *) string );		/* ditto   */

		i += 2;						/* skip the "re" */

		while ( string[i] != ':' )
			if ( string[i] == '\0' )
				return ( (char *) string );	/* no colon in string! */
			else
				i++;

		/*
		 * now we've gotten to the colon, skip to the next
		 * non-whitespace  
		 */

		i++;						/* past the colon */

		while ( whitespace(string[i]) )
			i++;

	} while ( string[i] == 'r' && string[i + 1] == 'e' );

	/*
	 * and now copy it into the skipped_buf and sent it along... 
	 */

	strcpy( skipped_buf, (char *) string + i );

	return ( (char *) skipped_buf );
}
