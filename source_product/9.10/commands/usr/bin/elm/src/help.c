/**			help.c				**/

/*

 *  @(#) $Revision: 72.1 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *   help routine for ELM program 
 */


#include <signal.h>

#include "headers.h"


#ifdef NLS
# define NL_SETN  21
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS

void
mm_help_menu()
{
	/* draw the main menu help prompt */

	if ( mini_menu ) {
		MoveCursor( LINES - 7, 0 );
		CleartoEOS();

		Centerline( LINES - 7, catgets(nl_fd,NL_SETN,1,"ELM Help System") );
		Centerline( LINES - 5, catgets(nl_fd,NL_SETN,2,"Press keys you want help for, '?' for a list, or '.' to end") );
	} else {
		MoveCursor( LINES - 3, 0 );
		CleartoEOS();
	}

	PutLine0( LINES - 3, 0, catgets(nl_fd,NL_SETN,3,"Help on key: ") );
}

int
help()

{
	/*
	 *  Process the request for help [section] from the user 
	 */


	char            ch;	/* character buffer for input */
	char            *s;	/* string pointer...	      */


	mm_help_menu();
#ifdef SIGWINCH
	push_state(WA_MHM);
#endif

	do {
		MoveCursor( LINES - 3, (int)strlen(catgets(nl_fd,NL_SETN,3,"Help on key: ")) );
		ch = (char)tolower( ReadCh() );

		if ( ch == '.' )
#ifdef SIGWINCH
				{
			pop_state();
#endif
			return ( 0 );		/* zero means footer rewrite only */
#ifdef SIGWINCH
		}
#endif

		s = catgets(nl_fd,NL_SETN,5,"Unknown command.  Use '?' for a list of commands..." );

		switch ( ch ) {

		case '?':
			display_helpfile( MAIN_HELP );
#ifdef SIGWINCH
			pop_state();
#endif
			return ( 1 );

	  	case '$':
	  		s = catgets(nl_fd,NL_SETN,6,"$ = Force a resync of the current mailbox.  This will 'purge' deleted mail");
	   		break;

		case '!':
			s = catgets(nl_fd,NL_SETN,7,"! = Escape to the Unix shell of your choice, or just to enter commands");
			break;

	/*
	 *  This is for internal support.
	 *
	 *	case '@':
	 *		s =
	 *			catgets(nl_fd,NL_SETN,8,"@ = Debug - display a summary of the messages on the header page");
	 *		break;
 	 */

		case '|':
			s = catgets(nl_fd,NL_SETN,9,"| = Pipe the current message or tagged messages to the command specified");
			break;

	/*
	 *  This is for internal support.
	 *
	 *	case '#':
	 *		s = catgets(nl_fd,NL_SETN,10,"# = Debug - display all information known about current message");
	 *		break;
	 *	case '%':
	 *		s = catgets(nl_fd,NL_SETN,11,"% = Debug - display the computed return address of the current message");
	 *		break;
	 */

		case '*':
			s = catgets(nl_fd,NL_SETN,12,"* = Go to the last message in the current mailbox");
			break;

		case '-':
			s = catgets(nl_fd,NL_SETN,13,"- = Go to the previous page of messages in the current mailbox");
			break;

		case '=':
			s = catgets(nl_fd,NL_SETN,14,"'=' = Go to the first message in the current mailbox");
			break;

		case ' ':
		case '+':
			s = catgets(nl_fd,NL_SETN,15,"+ = Go to the next page of messages in the current mailbox");
			break;

		case '/':
			s = catgets(nl_fd,NL_SETN,16,"/ = Search for specified pattern in mailbox");
			break;

		case '<':
			s = catgets(nl_fd,NL_SETN,17,"< = Scan current message for calendar entries (if enabled)");
			break;

	/*	case '>':
	 *		s = catgets(nl_fd,NL_SETN,18,"> = Save current message or tagged messages to specified file");
	 		break;
	 */
	/*
	 *  This is for internal support.
	 *
	 *	case '^':
	 *		s = catgets(nl_fd,NL_SETN,19,"^ = Toggle the Delete/Undelete status of the current message");
	 *		break;
	 */

		case 'a':
			s = catgets(nl_fd,NL_SETN,20,"a = Enter the alias sub-menu section.  Create and display aliases");
			break;

		case 'b':
			s = catgets(nl_fd,NL_SETN,21,"b = Bounce (remail) a message to someone as if you have never seen it");
			break;

		case 'c':
			s = catgets(nl_fd,NL_SETN,22,"c = Change mailboxes, leaving the current mailbox as if 'quitting'");
			break;

		case 'd':
			s = catgets(nl_fd,NL_SETN,23,"d = Mark the current message for future deletion");
			break;

		case ctrl('D'):
			s = catgets(nl_fd,NL_SETN,24,"^D = Mark for deletion all messages with the specified pattern");
			break;

	/*
	 *  This is for internal support.
	 *
	 *	case 'e':
	 *		s = catgets(nl_fd,NL_SETN,25,"e = Invoke the editor on the entire mailbox, resync'ing when done");
	 *		break;
	 */

		case 'f':
			s = catgets(nl_fd,NL_SETN,26,"f = Forward the current message to someone, return address is yours");
			break;

		case 'g':
			s = catgets(nl_fd,NL_SETN,27,"g = Group reply not only to the sender, but to everyone who received msg");
			break;

		case 'h':
			s = catgets(nl_fd,NL_SETN,28,"h = Display message with all Headers (ignore weedout list)");
			break;
			
		case 'j':
			s = catgets(nl_fd,NL_SETN,29,"j = Go to the next message.  This is the same as the DOWN arrow");
			break;

		case 'k':
			s = catgets(nl_fd,NL_SETN,30,"k = Go to the previous message.  This is the same as the UP arrow");
			break;

		case 'l':
			s = catgets(nl_fd,NL_SETN,31,"l = Limit displayed messages based on the specified criteria");
			break;

		case 'm':
			s = catgets(nl_fd,NL_SETN,32,"m = Create and send mail to the specified person or persons");
			break;

		case 'n':
			s = catgets(nl_fd,NL_SETN,33,"n = Read the next message, then move current to next messge");
			break;

		case 'o':
			s = catgets(nl_fd,NL_SETN,34,"o = Go to the options submenu");
			break;

		case 'p':
			s = catgets(nl_fd,NL_SETN,35,"p = Print the current message or the tagged messages");
			break;

		case 'q':
			s = catgets(nl_fd,NL_SETN,36,"q = Quit the mailer, asking about deletion, saving, etc");
			break;

		case 'r':
			s = catgets(nl_fd,NL_SETN,37,"r = Reply to the message.  This only sends to the originator of the message");
			break;

		case 's':
			s = catgets(nl_fd,NL_SETN,38,"s = Save current message or tagged messages to specified file");
			break;

		case 't':
			s = catgets(nl_fd,NL_SETN,39,"t = Tag a message for further operations (or untag if tagged)");
			break;

		case ctrl('T'):
			s = catgets(nl_fd,NL_SETN,40,"^T = Tag all messages with the specified pattern");
			break;

		case 'u':
			s = catgets(nl_fd,NL_SETN,41,"u = Undelete - remove the deletion mark on the message");
			break;

		case ctrl('U'):
			s = catgets(nl_fd,NL_SETN,50,"^U = Undelete - remove the 'D' mark on all msgs with the specified pattern");
			break;

		case 'x':
			s = catgets(nl_fd,NL_SETN,42,"x = Exit the mail system quickly");
			break;

		case '\n':
		case '\r':
			s = catgets(nl_fd,NL_SETN,43,"<return> = Read the current message");
			break;

		case ctrl('L'):
			s = catgets(nl_fd,NL_SETN,44,"^L = Rewrite the screen");
			break;

		case '\x7f': 	/* DEL, ^? */
		case ctrl('Q'):
			s = catgets(nl_fd,NL_SETN,45,"Exit the mail system quickly");
			break;

#ifdef SIGWINCH
		case (char)NO_OP_COMMAND:
			s = "";
			break;
#endif

		default:
			if (isdigit(ch))
				s = catgets(nl_fd,NL_SETN,46,"<number> = Make specified number the current message");
		}

		ClearLine( LINES - 1 );
		Centerline( LINES - 1, s );

	} while ( ch != '.' );

	/*
	 *  we'll never actually get here, but that's okay... *
	 */

#ifdef SIGWINCH
	pop_state();
#endif
	return ( 0 );
}

int 
display_helpfile( section )

	int             section;

{
	/*
	 * Help me!  Read file 'helpfile.<section>' and echo to screen 
	 */


	FILE            *hfile;
	char            buffer[SLEN];
	int             lines = 0;


#ifdef SIGWINCH
	push_state(WA_NOT);
#endif

#ifdef NLS
	sprintf( buffer, "%s/%s/elm/%s.%d", nl_helphome, lang, helpfile, section );
#else  NLS
	sprintf( buffer, "%s/%s.%d", helphome, helpfile, section );
#endif NLS


	if ( (hfile = fopen(buffer, "r")) == NULL ) {


#ifdef NLS
		sprintf( buffer, "%s/%s.%d", helphome, helpfile, section );
		if ( (hfile = fopen(buffer, "r")) == NULL ){
#endif NLS


		dprint( 1, (debugfile,
		        "Error: Couldn't open helpfile %s (help)\n", buffer) );
		error1( catgets(nl_fd,NL_SETN,47,"couldn't open helpfile %s"), buffer );
#ifdef SIGWINCH
		pop_state();
#endif
		return ( FALSE );


#ifdef NLS
		}
#endif NLS


	}

	ClearScreen();

	while ( fgets(buffer, SLEN, hfile) != NULL ) {
		if ( lines > LINES - 3 ) {
			PutLine0( LINES, 0, catgets(nl_fd,NL_SETN,48,"Press any key to continue: ") );

#ifdef SIGWINCH
			while(ReadCh() == NO_OP_COMMAND)
				;
#else
			(void) ReadCh();
#endif
			lines = 0;
			ClearScreen();
			Write_to_screen( "%s\r", 1, buffer, NULL, NULL );
		} else
			Write_to_screen( "%s\r", 1, buffer, NULL, NULL );

		lines++;
	}

	PutLine0( LINES, 0, catgets(nl_fd,NL_SETN,49,"Press any key to return: ") );

#ifdef SIGWINCH
	while(ReadCh() == NO_OP_COMMAND)
		;
#else
	(void) ReadCh();
#endif
	clear_error();
	fclose( hfile );
#ifdef SIGWINCH
	pop_state();
#endif

	return ( TRUE );
}
