/**			screen.c			**/

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
 *   screen display routines for ELM program 
 *
 */

#include "headers.h"

#define  minimum(a,b)	( (a) < (b) ? (a) : (b) )


#ifdef NLS
# include <nl_ctype.h>
# define NL_SETN  38
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


char		message_status[3];
int             last_current = -1;

extern char	shortened_name[STRING];

char		*nameof(), 
		*show_status();


int
showscreen()

{

	ClearScreen();

	update_title();    			/* display the title line    */

	last_header_page = -1;			/* force a redraw regardless */
	show_headers();

	if ( mini_menu )
		show_menu();

	show_last_error();

	if ( hp_terminal )
		define_softkeys( MAIN );
}


int
update_title()

{
	/*
	 *  display a new title line, probably due to new mail arriving 
	 */


	char            buffer[SLEN],
			*short_fname;
	int		wrap = 0,
			short_title = FALSE;


	short_fname = nameof( infile );

	do{
		if ( !short_title ){
			if ( selected )
				sprintf( buffer, catgets(nl_fd,NL_SETN,3,"Mailbox is '%s' with %d shown out of %d [Elm %s]"),
					short_fname, selected, message_count, revision_id );
			else {
				sprintf( buffer, catgets(nl_fd,NL_SETN,4,"Mailbox is '%s' with %d message(s) [Elm %s]"),
					 short_fname, message_count, revision_id );
		}
		} else 
			sprintf( buffer, catgets(nl_fd,NL_SETN,1,"Mailbox is '%s' [Elm %s]"),
				 short_fname, revision_id );

		/*
		* Fix for SR 5000698225 and DSDe411984 (DSDe411551 on S300)
		* "Oki/japanese/vt100 display problem" 
		* -> add two columns to the boundary <-
		* Note that tail_of_string() still handles MB chars incorrectly
		*/
		wrap = strlen( buffer ) - ( COLUMNS + 1 ) + 2;

		if ( wrap > 0 )
			if ( strlen(short_fname)-wrap > 4 )  /* 4 -> see tail_of_string() */
				short_fname 
				= tail_of_string( short_fname, (int)strlen(short_fname) - wrap );
			else {
				short_fname = nameof( infile );
				short_title = TRUE;
			}

	} while ( wrap > 0 );

	ClearLine( 1 );

	Centerline( 1, buffer );
}


int
show_menu()

{
	/*
	 *  write main system menu... 
	 */

	if ( user_level == 0 ) {	/* a rank beginner.  Give less options  */
		Centerline( LINES - 7,
			   catgets(nl_fd,NL_SETN,5,"You can use any of the following commands by pressing the first character;") );
		Centerline( LINES - 6,
			   catgets(nl_fd,NL_SETN,6,"D)elete or U)ndelete mail,  M)ail a message,  R)eply or F)orward mail,  Q)uit") );
		Centerline( LINES - 5,
			   catgets(nl_fd,NL_SETN,7,"To read a message, press <return>.  j = move down, k = move up, ? = help") );
	} else {
		Centerline( LINES - 7,
			   catgets(nl_fd,NL_SETN,8,"|=pipe, !=shell, ?=help, <n>=set current to n, /=search pattern") );
		Centerline( LINES - 6,
			   catgets(nl_fd,NL_SETN,9,"A)lias, C)hange mailbox, D)elete, F)orward, G)roup reply, M)ail, N)ext,") );
		Centerline( LINES - 5,
			   catgets(nl_fd,NL_SETN,10,"O)ptions, P)rint, R)eply, S)ave, T)ag, Q)uit, U)ndelete, or eX)it") );
	}
}


int
show_headers()

{
	/*
	 *  Display page of headers (10) if present.  First check to 
	 *  ensure that header_page is in bounds, fixing silently if not.
	 *  If out of bounds, return zero, else return non-zero 
	 *  Modified to only show headers that are "visible" to ze human
	 *  person using ze program, eh?
	 */


	register int    this_msg = 0, 
			line = 4, 
			last = 0, 
			last_line, 
			displayed = 0;
	char            newfrom[VERY_LONG_STRING], 
			buffer[VERY_LONG_STRING];


	if ( fix_header_page() )
		return ( FALSE );

	if ( selected ) {
		if ( (header_page * headers_per_page) > selected )
			return ( FALSE );

		this_msg = visible_to_index( header_page * headers_per_page + 1 );
		displayed = header_page * headers_per_page;

		last = displayed + headers_per_page - 1;

	} else {
		if ( header_page == last_header_page )	
			return ( FALSE );

		/*
		 *  compute last header to display 
		 */

		this_msg = header_page * headers_per_page;
		last = this_msg +  headers_per_page - 1 ;
	}

	if ( last >= message_count )
		last = message_count - 1;

	/*
	 *  Okay, now let's show the header page! 
	 */

	ClearLine( line );		/* Clear the top line... */

	MoveCursor( line, 0 );		/* and move back to the top of the page... */

	while ( (selected && displayed <= last) 
		 || (!selected && this_msg <= last) ) {
		tail_of( header_table[this_msg].from, newfrom, TRUE );

		if ( selected ) {
			if ( this_msg == current - 1 )
				build_header_line( buffer, &header_table[this_msg], ++displayed,
						  TRUE, newfrom );
			else
				build_header_line( buffer, &header_table[this_msg],
					       ++displayed, FALSE, newfrom );
		} else {
			if ( this_msg == current - 1 )
				build_header_line( buffer, &header_table[this_msg], this_msg + 1,
						  TRUE, newfrom );
			else
				build_header_line( buffer, &header_table[this_msg],
					      this_msg + 1, FALSE, newfrom );
		}

		Write_to_screen( "%s\r\n", 1, buffer, NULL, NULL );	/* avoid '%' probs */
		CleartoEOLN();
		line++;				/* for clearing up in a sec... */

		if ( selected ) {
			if ( (this_msg = next_visible(this_msg + 1) - 1) < 0 )
				break;		/* GET OUTTA HERE! */

			/*
			 * the preceeding looks gross because we're using an
			 * INDEX variable to pretend to be a "current"
			 * counter, and the current counter is always 1
			 * greater than the actual index.  Does that make
			 * sense?? 
			 */
		} else
			this_msg++;	
	}

	if ( mini_menu )
		last_line = LINES - 8;
	else
		last_line = LINES - 4;

	while ( line < last_line ) {
		ClearLine( line );
		line++;
	}

	display_central_message();

	last_current = current;
	last_header_page = header_page;

	return ( TRUE );
}


int
show_current()

{
	/*
	 *  Show the new header, with all the usual checks 
	 */


	register int    first = 0, 
			last = 0, 
			last_line, 
			new_line;
	int		curr_index,
			last_index;
	char            newfrom[VERY_LONG_STRING], 
			old_buffer[VERY_LONG_STRING], 
			new_buffer[VERY_LONG_STRING];


	(void) fix_header_page();	

	/*
	 *  compute last header to display 
	 */

	first = header_page * headers_per_page + 1;
	last = first + ( headers_per_page - 1 );

	if ( last > message_count )
		last = message_count;

	/*
	 *  okay, now let's show the pointers... 
	 *  have we changed??? 
	 */

	if ( current == last_current )
		return;

	if ( selected ) {
		curr_index = compute_visible(current - 1);
		last_index = compute_visible(last_current - 1);
	} else {
		curr_index = current;
		last_index = last_current;
	}

	last_line = ( (last_index - 1) % headers_per_page ) + 4;
	new_line = ( (curr_index - 1) % headers_per_page ) + 4;

	if ( has_highlighting && !arrow_cursor ) {

		/*
		 *  build the new header lines... 
		 */

		tail_of(header_table[current - 1].from, newfrom, TRUE);

		build_header_line( new_buffer, &header_table[current-1],
			           curr_index, TRUE, newfrom );

		if ( last_current > 0 &&
		    (last_index <= last && last_index >= first) ) {

			dprint( 5, (debugfile,
				"\nlast_current = %d ... clearing [1] before we add [2]\n",
				last_index) );
			dprint( 5, (debugfile, "first = %d, and last = %d\n\n",
				first, last) );

			/* 
			 * Clear the enhancement
			 */

			tail_of(header_table[last_current - 1].from, 
							newfrom, TRUE);

			build_header_line( old_buffer, &header_table[last_current-1],
			           		last_index, FALSE, newfrom );

			ClearLine( last_line );
			PutLine0( last_line, 0, old_buffer );
		}

		PutLine0( new_line, 0, new_buffer );
	
	} else {
		if ( on_page(last_index) )
			PutLine0( last_line, 0, "  " );	/* remove old pointer... */
		if ( on_page(curr_index) )
			PutLine0( new_line, 0, "->" );
	}

	last_current = current;
}


int
build_header_line( buffer, entry, message_number, highlight, from )

	char            *buffer;
	struct header_rec *entry;
	int             message_number, 
			highlight;
	char            *from;

{
	/*
	 *  Build in buffer the message header ... entry is the current
	 *  message entry, 'from' is a modified (displayable) from line, 
	 *  'highlight' is either TRUE or FALSE, and 'message_number'
	 *  is the number of the message.
	 * 
	 *
	 *  Note: using 'strncpy' allows us to output as much of the
	 *  subject line as possible given the dimensions of the screen.
	 *  The key is that 'strncpy' returns a 'char *' to the string
	 *  that it is handing to the dummy variable!  Neat, eh? 
	 */


	char            subj[LONG_SLEN];	/* to output subject */

#ifdef NLS
	int		w_flag[LONG_SLEN],      /* indicate 2-byte   */
			len,
			i;
#endif NLS


	strncpy( subj, entry->subject, (size_t)COLUMNS - 44 );

	subj[COLUMNS - 45] = '\0';		/* insurance, eh? */

#ifdef NLS

	/*
	 * To avoid from appearing the first byte character of 2-byte
	 * character at the last column of the line.
	 */

	len = strlen( subj );
	if ( len >= COLUMNS-45 ){
		w_flag[0] = FIRSTof2(subj[0])?1:0;
		for ( i=1; i<len; i++ )
			if ( w_flag[i-1] == 0 )
				w_flag[i] = FIRSTof2(subj[i])?1:0;
			else
				w_flag[i] = SECof2(subj[i])?2:0;
	
		if ( w_flag[COLUMNS-46] == 1 )
			subj[COLUMNS-46] = ' ';
	}

#endif NLS

	strip_tabs( subj );			/* to avoid wrapping around */

	subj[COLUMNS - 45] = '\0';		/* insurance, eh? */

	sprintf( buffer, "%s%s%s%c%-3d %3.3s %-2d %-18.18s (%d) %s%s%s",
		highlight ? ((has_highlighting && !arrow_cursor) ?
			     start_highlight : "->") : "  ",
		(highlight && has_highlighting && !arrow_cursor) ? "  " : "",
		show_status(entry->status),
		(entry->status & TAGGED ? '+' : ' '),
		message_number,
		entry->month,
		atoi(entry->day),
		from,
		entry->lines,
		(entry->lines / 1000 > 0 ? "" :		/* spacing the  */
		 entry->lines / 100 > 0 ? " " :		/* same for the */
		 entry->lines / 10 > 0 ? "  " :		/* lines in ()  */
		 "   "),				/* [wierd]    */
		subj,
		(highlight && has_highlighting && !arrow_cursor) ?
		end_highlight : "" );

}


int
fix_header_page()

{
	/*
	 *  this routine will check and ensure that the current header
	 *  page being displayed contains messages!  It will silently
	 *  fix 'header-page' if wrong.  Returns TRUE if changed. 
	 */


	int             last_page, 
			old_header;


	old_header = header_page;

	if ( selected )
		last_page = (int)( (selected-1)/headers_per_page );
	else
		last_page = (int) ( (message_count-1)/headers_per_page );

	if ( header_page > last_page )
		header_page = last_page;
	else if ( header_page < 0 )
		header_page = 0;

	return ( old_header != header_page );
}


int
on_page( message )

	int             message;

{
	/*
	 *  Returns true iff the specified message is on the displayed page. 
	 */


	if ( message >= header_page * headers_per_page + 1)
		if ( message <= ((header_page + 1) * headers_per_page) )
			return ( TRUE );

	return ( FALSE );
}


char  
*show_status( status )

	int             status;

{
	/*
	 *  This routine returns a pair of characters indicative of
	 *  the status of this message.  The first character represents
	 *  the interim status of the message (e.g. the status within 
	 *  the mail system):
 	 *
	 *	D = Deleted message
	 *	E = Expired message
	 *	N = New message
	 *	_ = (space) default 
  	 *
 	 *  and the second represents the permanent attributes of the
	 *  message:
  	 *
 	 *	C = Company Confidential message
 	 *      U = Urgent (or Priority) message
 	 *	P = Private message
	 *	A = Action associated with message
	 *	F = Form letter
	 *	_ = (space) default
	 */

	/*
	 *  the first character, please 
	 */

	if ( status & DELETED )
		message_status[0] = 'D';
	else if ( status & EXPIRED )
		message_status[0] = 'E';
	else if ( status & NEW )
		message_status[0] = 'N';
	else
		message_status[0] = ' ';

	/*
	 *  and the second... 
	 */

	if ( status & CONFIDENTIAL )
		message_status[1] = 'C';
	else if ( status & URGENT )
		message_status[1] = 'U';
	else if ( status & PRIVATE )
		message_status[1] = 'P';
	else if ( status & ACTION )
		message_status[1] = 'A';
	else if ( status & FORM_LETTER )
		message_status[1] = 'F';
	else
		message_status[1] = ' ';

	message_status[2] = '\0';

	return ( (char *) message_status );
}
