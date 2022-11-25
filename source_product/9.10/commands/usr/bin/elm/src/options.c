/**			options.c			**/

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
 *  This set of routines allows the alteration of a number of 
 *  parameters in the Elm mailer, including the following;
 *
 *	calendar-file	<where to put calendar entries>
 *	display pager	<how to page messages>
 *	editor		<name of composition editor>
 *	folder-dir	<folder directory>
 *	sort-by		<how to sort mailboxes>
 *	savefile	<file to save outbound message copies to>
 *	printmail	<how to print messages>
 *	full_username	<your full user name for outgoing mail>
 *
 *	arrow-cursor	<on or off>
 *	menu-display    <on or off>
 *
 *	user-level	<BEGINNER|INTERMEDIATE|EXPERT>
 *      names-only      <on or off>
 *      expand_tabs     <on or off>            <------- added
 *  And others as they seem useful.
 */


#include <ctype.h>
#include <signal.h>

#include "headers.h"

#undef onoff
#define   onoff(n)	( n == 1? "ON ":"OFF" )


#ifdef NLS
# define NL_SETN  29
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


char		*one_liner_for(), 
		*level_name();

#ifdef SIGWINCH
extern int    resized;        /* flag indicating window resize occurred */
#endif

int
options()

{
	/*
	 *  change options... 
	 */


	char            ch,
			prevdir[SLEN],
			buffer[SLEN];
	char           *sort_name();


	display_options();
#ifdef SIGWINCH
	resized = 0;
#endif

	do {
		ClearLine( LINES - 4 );

		Centerline( LINES - 4, catgets(nl_fd,NL_SETN,1,"Select first letter of Option line, '>' to Save, or R)eturn") );

		PutLine0( LINES - 2, 0, catgets(nl_fd,NL_SETN,2,"Command: ") );

		ch = (char)tolower( ReadCh() );

		clear_error();		/* remove possible "sorting" message etc... */

		one_liner( one_liner_for(ch) );

		switch ( ch ) {
		case 'c':
			strcpy( buffer, calendar_file );
			optionally_enter( buffer, 2, 23, FALSE );
			remove_spaces( buffer );
			expand_env( calendar_file, buffer ); 
			if ( *calendar_file == '\0' ) {
				error( catgets(nl_fd,NL_SETN,40,"Input is NULL, will use default value!") );
				sprintf( calendar_file, "%s/%s", home,
						   dflt_calendar_file );
				PutLine1( 2, 0,catgets(nl_fd,NL_SETN,47, "C)alendar file       : %s"),
				          calendar_file );
			}
			break;

		case 'd':
			PutLine0( 3, 0,catgets(nl_fd,NL_SETN,48, "D)isplay mail using  : ") );
			strcpy( buffer, pager );
			optionally_enter( buffer, 3, 23, FALSE );
			remove_spaces( buffer );
			expand_env( pager, buffer ); 
			if ( *pager == '\0' ) {
				error( catgets(nl_fd,NL_SETN,40,"Input is NULL, will use default value!") );
				strcpy( pager, default_pager );
				PutLine1( 3, 0,catgets(nl_fd,NL_SETN,49, "D)isplay mail using  : %s"), pager );
			}
			clear_pages = ( equal(pager, "builtin+") ||
				        equal(pager, "internal+") );
			break;

		case 'e':
			PutLine0( 4, 0,catgets(nl_fd,NL_SETN,50, "E)ditor              : ") );
			strcpy( buffer, editor );
			optionally_enter( buffer, 4, 23, FALSE );
			remove_spaces( buffer );
			expand_env( editor , buffer ); 
			if ( *editor == '\0' ) {
				error( catgets(nl_fd,NL_SETN,40,"Input is NULL, will use default value!") );
				strcpy( editor, default_editor );
				PutLine1( 4, 0,catgets(nl_fd,NL_SETN,51, "E)ditor              : %s"), editor );
			}
			break;

		case 'f':
			PutLine0( 5, 0,catgets(nl_fd,NL_SETN,52, "F)older directory    : ") );
			strcpy( prevdir, folders );
			strcpy( buffer, folders );
			optionally_enter( buffer, 5, 23, FALSE );
			remove_spaces( buffer );
			expand_env( folders, buffer ); 
			if ( *folders == '\0' ) {
				error( catgets(nl_fd,NL_SETN,40,"Input is NULL, will use default value!") );
				sprintf( folders, "%s/%s", home, "Mail" );
				PutLine1( 5, 0,catgets(nl_fd,NL_SETN,53, "F)older directory    : %s"),
					  folders );
			}
			if ( access(folders, 00) == -1 ) {
				if ( hp_terminal )
					define_softkeys( YESNO );
				PutLine1( LINES - 1, 0, catgets(nl_fd,NL_SETN,41,"Couldn't find the folder directory -- %s"),
					  folders );
				PutLine1( LINES, 0, catgets(nl_fd,NL_SETN,42,"Do you want to create the directory (y/n)? y%c"),
					  BACKSPACE );
				ch = (char) yesno( 'y', TRUE );
				if ( hp_terminal )
					softkeys_off();
				ClearLine( LINES - 1 );
				ClearLine( LINES );
				if ( ch == 'y' ) {
					if ( mkdir(folders, 0700) != 0 ) {
						error( catgets(nl_fd,NL_SETN,43,"Couldn't make the directory, will use default value!") );
						sprintf( folders, "%s/%s", home, "Mail" );
				                 PutLine1( 5, 0,catgets(nl_fd,NL_SETN,53, "F)older directory    : %s"),
						 folders );
					}
				} else if ( ch == 'n' ) {
					error( catgets(nl_fd,NL_SETN,44,"Will use default value!") );
					sprintf( folders, "%s/%s", home, "Mail" );
					PutLine1( 5, 0,catgets(nl_fd,NL_SETN,53, "F)older directory    : %s"),
						  folders );
				} else {
					error( catgets(nl_fd,NL_SETN,45,"Use previous value") );
					strcpy( folders, prevdir );
					PutLine1( 5, 0,catgets(nl_fd,NL_SETN,53, "F)older directory    : %s"),
						  folders );
				}
			}
			break;

		case 's':
			PutLine1( 6, 0,catgets(nl_fd,NL_SETN,54, "S)orting criteria    : %s"),
					sort_name(PAD) );
			CleartoEOLN();
			change_sort( 6, 23 );
			break;

		case 'o':
			PutLine0( 7, 0,catgets(nl_fd,NL_SETN,55, "O)utbound mail saved : ") );
			strcpy( buffer, savefile );
			optionally_enter( buffer, 7, 23, FALSE );
			remove_spaces( buffer );
			expand_env( savefile, buffer ); 
			if ( *savefile == '\0' ) {
				error( catgets(nl_fd,NL_SETN,40,"Input is NULL, will use default value!") );
				sprintf( savefile, "%s/%s", home, "mbox" );
				PutLine1( 7, 0,catgets(nl_fd,NL_SETN,56, "O)utbound mail saved : %s"), savefile );
			}
			break;

		case 'p':
			PutLine0( 8, 0,catgets(nl_fd,NL_SETN,57, "P)rint mail using    : ") );
			optionally_enter( printout, 8, 23, FALSE );
			remove_spaces( printout );
			if ( *printout == '\0' ) {
				error( catgets(nl_fd,NL_SETN,40,"Input is NULL, will use default value!") );
				strcpy( printout, "pr %s | lp" );
				PutLine1( 8, 0,catgets(nl_fd,NL_SETN,58, "P)rint mail using    : %s"), printout );
			}
			break;

		case 'y':
			PutLine0( 9, 0,catgets(nl_fd,NL_SETN,59, "Y)our full name      : ") );
			optionally_enter( full_username, 9, 23, FALSE );
			remove_spaces( full_username );
			if ( *full_username == '\0' ) {
				error( catgets(nl_fd,NL_SETN,40,"Input is NULL, will use default value!") );
				strcpy( full_username, dflt_full_name );
				PutLine1( 9, 0,catgets(nl_fd,NL_SETN,60, "Y)our full name      : %s"), full_username );
			}
			break;

		case 'a':
			PutLine1( 12, 0,catgets(nl_fd,NL_SETN,61, "A)rrow cursor        : %s"),
					onoff(arrow_cursor) );
			CleartoEOLN();
			on_or_off( &arrow_cursor, 12, 23 );
			break;

		case 'm':
			on_or_off( &mini_menu, 13, 23 );
			headers_per_page = LINES - ( mini_menu ? 13 : 8 );
			get_page( current );
			break;

		case 'u':
			switch_user_level( &user_level, 15, 23 );
			break;

		case 'n':
			on_or_off( &names_only, 16, 23 );
			break;

		case 't':
			on_or_off( &expand_tabs, 17, 23 );
			break;

		case '?':
			options_help();
			MoveCursor( LINES - 3, 0 );
			CleartoEOS();
			PutLine0( LINES - 2, 0, catgets(nl_fd,NL_SETN,2,"Command: ") );
			break;

		case '>':
			printf( catgets(nl_fd,NL_SETN,3,"Save options in .elm/elmrc...") );
			fflush( stdout );
			save_options();
			break;

		case 'x':
		case 'r':
		case ctrl( 'M' ):
		case ctrl( 'J' ):
			return;

		case ctrl( 'L' ):
			display_options();
			break;

#ifdef SIGWINCH
		case (char)NO_OP_COMMAND:
			break;
#endif

		default:
			error( catgets(nl_fd,NL_SETN,4,"Command unknown!") );
		}
#ifdef SIGWINCH
		if (resized) {
			resized = 0;
			display_options();
			show_last_error();
		}
#endif

	} while ( ch != 'r' );

}

int
display_options()

{
	/*
	 *  Display all the available options.. 
	 */


	int		clear_below;
	char           *sort_name();


	ClearScreen();
	Centerline( 0, catgets(nl_fd,NL_SETN,5,"-- Elm Options Editor --") );


#ifdef ENABLE_CALENDAR
	PutLine1( 2, 0,catgets(nl_fd,NL_SETN,47, "C)alendar file       : %s"), calendar_file );
	if ( strlen(calendar_file)+23 > COLUMNS ) {
		MoveCursor( 3, 0 );
		CleartoEOS();
	}
#endif


	PutLine1( 3, 0,catgets(nl_fd,NL_SETN,48, "D)isplay mail using  : %s"), pager );
	if ( strlen(pager)+23 > COLUMNS ) {
		MoveCursor( 4, 0 );
		CleartoEOS();
	}

	PutLine1( 4, 0,catgets(nl_fd,NL_SETN,51, "E)ditor              : %s"), editor );
	if ( strlen(editor)+23 > COLUMNS ) {
		MoveCursor( 5, 0 );
		CleartoEOS();
	}

	PutLine1( 5, 0,catgets(nl_fd,NL_SETN,53, "F)older directory    : %s"), folders );
	if ( strlen(folders)+23 > COLUMNS ) {
		MoveCursor( 6, 0 );
		CleartoEOS();
	}

	PutLine1( 6, 0,catgets(nl_fd,NL_SETN,54, "S)orting criteria    : %s"), sort_name(PAD) );

	PutLine1( 7, 0,catgets(nl_fd,NL_SETN,56, "O)utbound mail saved : %s"), savefile );
	if ( strlen(savefile)+23 > COLUMNS ) {
		MoveCursor( 8, 0 );
		CleartoEOS();
	}

	PutLine1( 8, 0,catgets(nl_fd,NL_SETN,58, "P)rint mail using    : %s"), printout );
	if ( strlen(printout)+23 > COLUMNS ) {
		MoveCursor( 9, 0 );
		CleartoEOS();
	}

	PutLine1( 9, 0,catgets(nl_fd,NL_SETN,60, "Y)our full name      : %s"), full_username );
	if ( strlen(full_username)+23 > COLUMNS ) {
		MoveCursor( 10, 0 );
		CleartoEOS();
	}

	PutLine1( 12, 0,catgets(nl_fd,NL_SETN,61, "A)rrow cursor        : %s"), onoff(arrow_cursor) );
	PutLine1( 13, 0,catgets(nl_fd,NL_SETN,62, "M)enu display        : %s"), onoff(mini_menu) );

	PutLine1( 15, 0,catgets(nl_fd,NL_SETN,63, "U)ser level          : %s"), level_name(user_level) );
	PutLine1( 16, 0,catgets(nl_fd,NL_SETN,64, "N)ames only          : %s"), onoff(names_only) );
	PutLine1( 17, 0,catgets(nl_fd,NL_SETN,65, "T)abs to spaces      : %s"), onoff(expand_tabs) );
}


int
on_or_off( var, x, y )

	int             *var, 
			x, 
			y;

{
	/*
	 *  'var' field at x.y toggles between on and off... 
	 */


	char            ch;


	PutLine0( x, y + 6,
	 	  catgets(nl_fd,NL_SETN,6,"(use <space> to toggle, any other key to leave)") );

	MoveCursor( x, y + 3 );				/* at end of value... */

	do {
		ch = ReadCh();

		if ( ch == SPACE ) {
			*var = !*var;
			PutLine0( x, y, onoff(*var) );
		}

#ifdef SIGWINCH
	} while(ch == SPACE || ch == (char)NO_OP_COMMAND);
#else
	} while ( ch == SPACE );
#endif

	MoveCursor( x, y + 4 );
	CleartoEOLN();					/* remove help prompt */
}


int
switch_user_level( ulevel, x, y )

	int             *ulevel, 
			x, 
			y;

{
#ifdef SIGWINCH
	char	ch;
#endif

	/*
	 *  step through possible user levels...
	 */

	PutLine0( x, y + 28, catgets(nl_fd,NL_SETN,7,"<space> to change") );

	MoveCursor( x, y );				/* at end of value... */

#ifdef SIGWINCH
	while((ch = ReadCh()) == ' '  || ch == (char)NO_OP_COMMAND) {
		if (ch == ' ') {
#else
	while ( ReadCh() == ' ' ) {
#endif
			*ulevel = ( *ulevel == 2 ? 0 : *ulevel + 1 );
			PutLine1( x, y, "%s", level_name(*ulevel) );
#ifdef SIGWINCH
		}
#endif
	}

	MoveCursor( x, y + 28 );
	CleartoEOLN();					/* remove help prompt */
}


int 
change_sort( x, y )

	int             x, 
			y;

{
	/*
	 *  change the sorting scheme... 
	 */


	int             last_sortby,	/* so we know if it changes... */
	                sign = 1;	/* are we reverse sorting??    */
	char            ch;		/* character typed in ...      */


	last_sortby = sortby;		/* remember current ordering   */

	PutLine0( x, COLUMNS - 45, catgets(nl_fd,NL_SETN,8,"(<space> for next, or R)everse)") );
	sort_one_liner( sortby );
	MoveCursor( x, y );

	do {
		ch = (char)tolower( ReadCh() );

		switch ( ch ) {
		case SPACE:
			if ( sortby < 0 ) {
				sign = -1;
				sortby = -sortby;
			} else
				sign = 1;			/* insurance! */

			sortby = sign * ( (sortby + 1) % (STATUS + 2) );

			if ( sortby == 0 )
				sortby = sign;	

			PutLine0( x, y, sort_name(PAD) );
			sort_one_liner( sortby );
			MoveCursor( x, y );
			break;

		case 'r':
			sortby = -sortby;
			PutLine0( x, y, sort_name(PAD) );
			sort_one_liner( sortby );
			MoveCursor( x, y );
		}

#ifdef SIGWINCH
	} while(ch == SPACE || ch == 'r' || ch == (char)NO_OP_COMMAND);
#else
	} while ( ch == SPACE || ch == 'r' );
#endif

	MoveCursor( x, COLUMNS - 30 );
	CleartoEOLN();

	if ( sortby != last_sortby ) {
		error( catgets(nl_fd,NL_SETN,9,"resorting mailbox...") );
		sort_mailbox( message_count, FALSE, 0 );
	}

	ClearLine( LINES - 2 );				/* clear sort_one_liner()! */
}


int
one_liner( string )

	char           *string;

{
	/*
	 *  A single-line description of the selected item... 
	 */

	ClearLine( LINES - 4 );
	Centerline( LINES - 4, string );
}


int
sort_one_liner( sorting_by )

	int             sorting_by;

{
	/*
	 *  A one line summary of the particular sorting scheme... 
	 */

	ClearLine( LINES - 2 );

	switch ( sorting_by ) {

	case -SENT_DATE:
		Centerline( LINES - 2,
			   catgets(nl_fd,NL_SETN,10,"This sort will order most-recently-sent to least-recently-sent") );
		break;
	case -RECEIVED_DATE:
		Centerline( LINES - 2,
			   catgets(nl_fd,NL_SETN,11,"This sort will order most-recently-received to least-recently-received") );
		break;
	case -MAILBOX_ORDER:
		Centerline( LINES - 2,
			   catgets(nl_fd,NL_SETN,12,"This sort will order most-recently-added-to-mailbox to least-recently") );
		break;
	case -SENDER:
		Centerline( LINES - 2,
			   catgets(nl_fd,NL_SETN,13,"This sort will order by sender name, in reverse alphabetical order") );
		break;
	case -SIZE:
		Centerline( LINES - 2,
		    	   catgets(nl_fd,NL_SETN,14,"This sort will order messages by longest to shortest") );
		break;
	case -SUBJECT:
		Centerline( LINES - 2,
			   catgets(nl_fd,NL_SETN,15,"This sort will order by subject, in reverse alphabetical order") );
		break;
	case -STATUS:
		Centerline( LINES - 2,
			   catgets(nl_fd,NL_SETN,16,"This sort will order by reverse status - Deleted through Tagged...") );
		break;

	case SENT_DATE:
		Centerline( LINES - 2,
			   catgets(nl_fd,NL_SETN,17,"This sort will order least-recently-sent to most-recently-sent") );
		break;
	case RECEIVED_DATE:
		Centerline( LINES - 2,
			   catgets(nl_fd,NL_SETN,18,"This sort will order least-recently-received to most-recently-received") );
		break;
	case MAILBOX_ORDER:
		Centerline( LINES - 2,
			   catgets(nl_fd,NL_SETN,19,"This sort will order least-recently-added-to-mailbox to most-recently") );
		break;
	case SENDER:
		Centerline( LINES - 2,
			   catgets(nl_fd,NL_SETN,20,"This sort will order by sender name") );
		break;
	case SIZE:
		Centerline( LINES - 2,
		      	   catgets(nl_fd,NL_SETN,21,"This sort will order messages by shortest to longest") );
		break;
	case SUBJECT:
		Centerline( LINES - 2,
			   catgets(nl_fd,NL_SETN,22,"This sort will order messages by subject") );
		break;
	case STATUS:
		Centerline( LINES - 2,
			   catgets(nl_fd,NL_SETN,23,"This sort will order by status - Tagged through Deleted...") );
		break;
	}
}


char  
*one_liner_for( c )

	char            c;

{
	/*
	 *  returns the one-line description of the command char... 
	 */

	switch ( c ) {
	case 'c':
		return ( 
			catgets(nl_fd,NL_SETN,24,"This is the file where calendar entries from messages are saved.") );

	case 'd':
		return ( 
			catgets(nl_fd,NL_SETN,25,"This is the program invoked to display individual messages (try 'builtin')") );

	case 'e':
		return ( 
			catgets(nl_fd,NL_SETN,26,"This is the editor that will be used for sending messages, etc.") );

	case 'f':
		return ( 
			catgets(nl_fd,NL_SETN,27,"This is the folders directory used when '=' (etc) is used in filenames") );

	case 'm':
		return ( 
			catgets(nl_fd,NL_SETN,28,"This determines if you have the mini-menu displayed or not") );

	case 'n':
		return ( 
			catgets(nl_fd,NL_SETN,29,"Whether to display the names and addresses on mail, or names only") );
	case 'o':
		return ( 
			catgets(nl_fd,NL_SETN,30,"This is where copies of outbound messages are saved automatically.") );

	case 'p':
		return ( 
			catgets(nl_fd,NL_SETN,31,"This is how printouts are generated.  \"%s\" will be replaced by filename.") );

	case 's':
		return ( 
			catgets(nl_fd,NL_SETN,32,"This is used to specify the sorting criteria for the mailboxes") );

	case 'y':
		return ( 
			catgets(nl_fd,NL_SETN,33,"When mail is sent out, this is what your full name will be recorded as.") );

	case 'a':
		return ( 
			catgets(nl_fd,NL_SETN,34,"This defines whether the ELM cursor is an arrow or a highlight bar.") );

	case 'u':
		return ( 
			catgets(nl_fd,NL_SETN,35,"The level of knowledge you have about the Elm mail system.") );

	case 't':
		return ( 
			catgets(nl_fd,NL_SETN,46,"When mail is sent out, expands tabs to spaces or not.") );

	default:
		return ( (char *)NULL );		/* nothing if we don't know! */
	}
}


void
o_help_menu()
{
	/* draw the options help menu */

	Centerline( LINES - 3,
		   catgets(nl_fd,NL_SETN,36,"Enter the key you want help on, '?' for a list, or '.' to exit help") );

	SetCursor( LINES -2, 0 );
	CleartoEOLN();

	lower_prompt( catgets(nl_fd,NL_SETN,37,"Key : ") );
}

int
options_help()

{
	/*
	 *  help menu for the options screen... 
	 */


	char            c, 
			*ptr;


#ifdef SIGWINCH
	push_state(WA_OHM);
#endif
	o_help_menu();

	while ( (c = (char)tolower(ReadCh())) != '.' ) {

		if ( c == '?' ) {
			display_helpfile( OPTIONS_HELP );
			display_options();
#ifdef SIGWINCH
			pop_state();
#endif
			return;
		}
#ifdef SIGWINCH
		if (c == (char)NO_OP_COMMAND)
			continue;
#endif

		if ( c == ctrl( 'J' ) || c== ctrl( 'M' ) )
			error1( catgets(nl_fd,NL_SETN,39,"%s = Return to main screen with the option values."),
				"<return>" );
		else if ( (ptr = one_liner_for(c)) != NULL )
			error2( "%c = %s", c, ptr );
		else
			error1( catgets(nl_fd,NL_SETN,38,"%c isn't used in this section"), c );

		lower_prompt( catgets(nl_fd,NL_SETN,37,"Key : ") );
	}
#ifdef SIGWINCH
	pop_state();
#endif
}


char 
*level_name( n )

	int             n;

{
	/*
	 *  return the 'name' of the level... 
	 */

	switch ( n ) {
	case 0:
		return ( "0 (for Beginning User)   " );
	case 1:
		return ( "1 (for Intermediate User)" );
	default:
		return ( "2 (for Expert User)      " );
	}
}
