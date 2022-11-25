/**			showmsgcmd.c			**/

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
 *  This is an interface for the showmsg command line.  The possible
 *  functions that could be invoked from the showmsg command line are
 *  almost as numerous as those from the main command line and include
 *  the following;
 *
 *	   |    = pipe this message to command...
 *	   !    = call Unix command
 *	   <    = scan message for calendar info
 *	   b    = bounce (remail) message
 *	   d    = mark message for deletion
 *	   f    = forward message
 *	   g    = group reply
 *	   h    = redisplay this message from line #1, showing headers
 *	   i    = move back to the index page (simply returns from function)
 *	   j,n  = move to body of next message
 *	   k    = move to body of previous message
 *	   m    = mail a message out to someone
 *	   p    = print this (all tagged) message
 *	   r    = reply to this message
 *	   s    = save this message to a maibox/folder 
 *	   t    = tag this message
 *	   u    = undelete message
 *	   x    = Exit Elm NOW 
 *
 *  all commands not explicitly listed here are beeped at.  Use I)ndex
 *  to get back to the main index page, please.
 *
 *  This function returns when it is ready to go back to the index
 *  page.
 */


#include <signal.h>

#include "headers.h"


#ifdef NLS
# define NL_SETN  40
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


int             screen_mangled = 0;


int
process_showmsg_cmd( command )

	int             command;

{
	int             i, 
			intbuf;			/* for dummy parameters...etc */
	char            error_line[LONG_SLEN];	/* for stat line messsages    */


	error_line[0] = '\0';

	Raw( ON );

	while ( TRUE ) {
		clear_error();
		switch ( command ) {

		case '|':
			clear_bottom_of_screen();
			PutLine0( LINES - 3, 0, catgets(nl_fd,NL_SETN,1,"Command: pipe") );
			(void) do_pipe();	/* do pipe - ignore return
						 * val */
			break;

		case '!':
			clear_bottom_of_screen();
			PutLine0( LINES - 3, 0, catgets(nl_fd,NL_SETN,2,"Command: system call") );
			(void) subshell();	/* do shell regardless */
			break;

		case '<':

#ifdef ENABLE_CALENDAR
			scan_calendar();
			break;
#else

			if ( screen_mangled )
				error( catgets(nl_fd,NL_SETN,3,"can't scan for calendar entries!") );
			else {
				ClearLine( LINES - 1 );
				PutLine0( LINES-1, 0,
					  catgets(nl_fd,NL_SETN,3,"can't scan for calendar entries!" ));
				CleartoEOLN();
			}

			break;
#endif

		case '%':
			clear_bottom_of_screen();
			get_return( error_line, FALSE );
			error1( "%s", error_line );
			break;

		case 'b':
			clear_bottom_of_screen();
			PutLine0( LINES - 3, 0, catgets(nl_fd,NL_SETN,4,"Command: bounce message") );
			remail();
			break;

		case 'd':
			delete_msg( TRUE, FALSE );	/* really delete it,
							 * silent */
			if ( !resolve_mode ) {
				if ( screen_mangled )
					strcpy( error_line, 
						catgets(nl_fd,NL_SETN,5,"message marked for deletion") );
				else {
					ClearLine( LINES - 1 );
					PutLine0( LINES - 1, 0, catgets(nl_fd,NL_SETN,6,"Message marked for deletion.  Command ? ") );
					CleartoEOLN();
				}
			} else
				goto move_to_next_message;
			break;

		case 'f':
			clear_bottom_of_screen();
			PutLine0( LINES - 3, 0, catgets(nl_fd,NL_SETN,7,"Command: forward message") );
			(void) forward();

			if ( resolve_mode )
				goto  move_to_next_message;

			break;

		case 'g':
			clear_bottom_of_screen();
			PutLine0( LINES - 3, 0, catgets(nl_fd,NL_SETN,8,"Command: group reply") );
			(void) reply_to_everyone();
			break;

		case 'h':
			screen_mangled = 0;
			if ( filter ) {
				filter = 0;
				intbuf = show_msg( current );
				filter = 1;
				return ( intbuf );
			} else
				return ( show_msg(current) );

	move_to_next_message:	/* a target for resolve mode actions */

		case 'j':
		case 'n':
			i = current;

			move_to_next();

			if ( current <= message_count ) {
				screen_mangled = 0;
				clearit( header_table[current-1].status, NEW );
				return( show_msg(current) );
			} else {
				current = i;
				return( 0 );
			}

		case 'k':
			i = current;

			move_to_prev();

			if ( current > 0 ) {
				screen_mangled = 0;
				clearit( header_table[current-1].status, NEW );
				return( show_msg(current) );
			} else {
				current = i;
				return( 0 );
			}

		case 'm':
			clear_bottom_of_screen();
			PutLine0( LINES - 3, 0, catgets(nl_fd,NL_SETN,9,"Command: Mail message") );

			if ( send_msg( "", "", "", TRUE, allow_forms, FALSE ) )
				show_last_error();

			break;

		case 'p':
			print_msg();

			if ( ! screen_mangled ) {
				ClearLine( LINES - 1 );
				PutLine0( LINES - 1, 0,
					 catgets(nl_fd,NL_SETN,11,"Queued for printing.  Command ? ") );
				CleartoEOLN();
			}

			break;

		case 'r':
			clear_bottom_of_screen();
			PutLine0( LINES - 3, 0, catgets(nl_fd,NL_SETN,12,"Command: reply to message") );
			(void) reply();
			break;

		case 's':
			clear_bottom_of_screen();
			PutLine0( LINES - 3, 0, catgets(nl_fd,NL_SETN,13,"Command: save message") );
			if ( save(&intbuf, TRUE) )
			        if ( resolve_mode )
				        goto move_to_next_message;
			break;       
			
		case 't':
			tag_message( FALSE );

			if ( screen_mangled ) {
				error( catgets(nl_fd,NL_SETN,14,"message tagged") );
			} else {
				ClearLine( LINES - 1 );
				PutLine0( LINES - 1, 0,
					 catgets(nl_fd,NL_SETN,15,"Message tagged.  Command ? ") );
				CleartoEOLN();
			}
			break;

		case 'u':
			undelete_msg( FALSE );	/* undelete it, silently */
			if ( !resolve_mode ) {
				if ( screen_mangled )
					error( catgets(nl_fd,NL_SETN,16,"message undeleted") );
				else {
					ClearLine( LINES - 1 );
					PutLine0( LINES - 1, 0,
					  catgets(nl_fd,NL_SETN,17,"Message undeleted.  Command ? ") );
					CleartoEOLN();
				}
			} else
				goto move_to_next_message;
			break;

		case 'x':
			fflush( stdout );
			leave( 0 );

		case 'q':
		case 'i':
			(void) get_page( current );
			clear_error();		/* zero out pending msg   */
			return ( 0 );		/* avoid <return> looping */

		case ' ':
		case '\n':
		case '\r':
		case ctrl( 'L' ):
			screen_mangled = 0;
			return ( show_msg(current) );

#ifdef SIGWINCH
		case (char)NO_OP_COMMAND:
			break;
#endif

		default:
			(void) putchar( (char) 007 );	/* BEEP! */
			fflush( stdout );

			if ( ! screen_mangled ) {
				ClearLine( LINES - 1 );
				PutLine0( LINES - 1, 0, 
					catgets(nl_fd,NL_SETN,18,"Request (return to I)ndex page) ? ") );
				CleartoEOLN();
			}

			break;
		}

		if ( screen_mangled ) {
			clear_bottom_of_screen();
			PutLine0( LINES - 3, 0, 
				  catgets(nl_fd,NL_SETN,18,"Request (return to I)ndex page) ? ") );

			if ( hp_terminal )
				define_softkeys( READ );

			if ( error_line[0] != '\0' ){
				error( error_line );
				error_line[0] = '\0';
			}
		} 

		command = (char)tolower( ReadCh() );
	}
}


int
clear_bottom_of_screen()

{
	/*
	 *  clear the last 4 lines of the screen... 
	 */

	screen_mangled = 1;

	MoveCursor( LINES - 4, 0 );
	CleartoEOS();
	PutLine0( LINES - 4, 0,
		 "--------------------------------------------------------------------------\n" );

	show_last_error();
}
