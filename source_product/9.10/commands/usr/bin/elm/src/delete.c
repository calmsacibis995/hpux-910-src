/**			delete.c			**/

/*
 *  @(#) $Revision: 66.1 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *   Delete or undelete files: just set flag in header record! 
 *   Also tags specified message(s)...
 */


#include "headers.h"


char           	*show_status(),
		*strcpy();

extern char	message_status[3];

int 
delete_msg( real_del, update_screen )

	int             real_del, 
			update_screen;

{
	/*
	 *  Delete current message.  If real-del is false, then we're
	 *  actually requested to toggle the state of the current
	 *  message... 
	 */


	if ( real_del )
		header_table[current - 1].status |= DELETED;
	else if ( ison(header_table[current - 1].status, DELETED) )
		clearit( header_table[current - 1].status, DELETED );
	else
		setit( header_table[current - 1].status, DELETED );

	if ( update_screen )
		show_msg_status( current - 1 );
}


int 
undelete_msg( update_screen )

	int             update_screen;

{
	/*
	 *  clear the deleted message flag 
	 */


	clearit( header_table[current - 1].status, DELETED );

	if ( update_screen )
		show_msg_status( current - 1 );
}


int 
show_msg_status( msg )

	int             msg;

{
	/*
	 *  show the status of the current message only.  
	 */


	char            tempbuf[3];


	strcpy( tempbuf, show_status(header_table[msg].status) );

	if ( selected )
		msg = compute_visible( msg ) - 1;

	if ( on_page(msg + 1) ) {
		MoveCursor( (msg % headers_per_page) + 4, 2 );
		Writechar( tempbuf[0] );
	}
}


int 
tag_message( screen )

	int		screen;

{
	/*
	 *  Tag current message.  If already tagged, untag it. 
	 */


	if ( ison(header_table[current - 1].status, TAGGED) )
		clearit( header_table[current - 1].status, TAGGED );
	else
		setit( header_table[current - 1].status, TAGGED );

	if ( screen )
		show_msg_tag( current - 1 );
}


int 
show_msg_tag( msg )

	int             msg;

{
	/*
	 *  show the tag status of the current message only.  
	 */
	
	int		org;


	org = msg;

	if ( selected )
		msg = compute_visible(msg) - 1;

	if ( on_page(msg + 1) ) {
		MoveCursor( (msg % headers_per_page) + 4, 4 );
		Writechar( ison(header_table[org].status, TAGGED) ? '+' : ' ' );
	}
}


int 
show_new_status( msg )

	int             msg;

{
	/*
	 *  If the specified message is on this screen, show
	 *  the new status (could be marked for deletion now,
	 *  and could have tag removed...)
	 */


	int		org;
	char		tempbuf[3];


	org = msg;
	strcpy( tempbuf, show_status(header_table[msg].status) );

	if ( selected )
		msg = compute_visible( msg ) - 1;

	if ( on_page(msg + 1) )
		PutLine2( (msg % headers_per_page) + 4, 2, "%s%c",
			  tempbuf,
			  ison(header_table[org].status, TAGGED) ? '+' : ' ' );
}
