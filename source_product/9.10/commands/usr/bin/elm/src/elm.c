/**			elm.c				**/

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
 * Main program of the ELM mail system! 
 */

#include <termio.h>
#include <signal.h>

#include "elm.h"


#ifdef NLS
# define NL_SETN   13
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


extern int	last_current;

int 	num_char = 0;

main(argc, argv)

	int             argc;
	char           *argv[];

{
	char            ch, 
			buff[SLEN],
			address[VERY_LONG_STRING], 
			to_whom[VERY_LONG_STRING];
	int             redraw,		/** do we need to rewrite the entire screen? **/
	                nuhead,		/** or perhaps just the headers section...   **/
	                nucurr,		/** or just the current message pointer...   **/
	                nufoot,		/** clear lines 16 thru bottom and new menu  **/
	                last_selected,	/** last selected messages by limit command  **/
	                i,		/** Random counting variable (etc)           **/
	                pageon,		/** for when we receive new mail...          **/
	                last_in_mailbox;/** for when we receive new mail too...      **/
	long            num;		/** another variable for fun..               **/

	parse_arguments( argc, argv, to_whom );

	if ( mail_only ) {

		initialize( FALSE );
		
		Raw( ON );
		dprint( 3, (debugfile, "Mail-only: mailing to\n-> \"%s\"\n",
			   format_long(to_whom, 3)) );
		(void) send_msg( to_whom, "", "", TRUE, NO, FALSE );
		leave( 0 );
	}

	initialize( TRUE );

	if ( check_size )
		check_mailfile_size();

	ScreenSize( &LINES, &COLUMNS );

	showscreen();

	Raw( ON );

	ch='x';		/* not NO_OP_COMMAND */
	while ( 1 ) {

		redraw = 0;
		nuhead = 0;
		nufoot = 0;
		nucurr = 0;
		
		if ( (num = bytes(infile)) != mailfile_size ) {
			dprint( 2, (debugfile, "Just received %d bytes more mail...\n",
				   num - mailfile_size) );
			error( catgets(nl_fd,NL_SETN,1,"New mail has arrived!") );
			fflush( stdin );		/* just to be sure... */
			ch='x';				/* not NO_OP_COMMAND */
			last_in_mailbox = message_count;
			pageon = header_page;

            /* exit right away if we cannot read infile for any reason */
            if (newmbox(2, FALSE, TRUE) != TRUE) { /* last won't be * touched! */
              clear_error();
              error1(catgets(nl_fd,NL_SETN,57,"Could not open file: %s \n"),
                   infile);
              silently_exit(1);
            }

			header_page = pageon;

			if ( on_page(current) )		/* do we REALLY have to
							 * rewrite? */
				showscreen();
			else {
				update_title();
				ClearLine( LINES - 1 );	/* remove reading
							 * message... */
				error1( catgets(nl_fd,NL_SETN,2,"%d new message(s) received" ),
				        message_count - last_in_mailbox );
			}

			if ( has_transmit )
				transmit_functions( ON );	/* insurance */
			clear_error();
                } else if ( (num = bytes(infile)) < mailfile_size ) {
#ifdef lint
		        num = num;
#endif
			dprint( 2, (debugfile, "Just deleted %d bytes mail...\n",
			 	   num - mailfile_size) );
		        error( catgets(nl_fd,NL_SETN,53,"Mailbox has modified! --- resyncing") );
                        fflush( stdin );                /* just to be sure... */
			(void) sleep(2);
			last_in_mailbox = message_count;
			pageon = header_page;
			newmbox(3, TRUE, TRUE); /* last won't be 
							* touched! */
		        header_page = pageon;

			if ( on_page(current) )         /* do we REALLY have to 
							* rewrite? */ 
				showscreen();
		 	else {
				update_title();
			 	ClearLine( LINES - 1 ); /* remove reading
						         * message... */
				error1( catgets(nl_fd,NL_SETN,2,"%d new message(s) received" ),
				 	message_count - last_in_mailbox);
			}
			if ( has_transmit )
				transmit_functions( ON );       /* insurance */
				clear_error();
		}


		/* Don't re-prompt if no activity */
#ifdef SIGWINCH
		if ((ch != NO_OP_COMMAND) || (resized)) {
#else
		if (ch!=NO_OP_COMMAND) {
#endif
			prompt( catgets(nl_fd,NL_SETN,3,"Command: ") );
			CleartoEOLN();
#ifdef SIGWINCH
			resized = 0;
#endif
		}

		ch = GetPrompt();

#ifdef SIGWINCH
		if ((ch != NO_OP_COMMAND) || (resized))
#else
		if (ch!=NO_OP_COMMAND)
#endif
			CleartoEOS();


#ifdef DEBUG
		if ( !movement_command(ch) )
			dprint( 4, (debugfile, "\nCommand: %c [%d]\n\n", ch, ch) );
#endif


		set_error( "" );			/* clear error buffer */

		num_char = strlen(catgets(nl_fd,NL_SETN,4,"Command: "));
		MoveCursor( LINES - 3, num_char );

		switch ( ch ) {

		case '?':
			if ( help() )
				redraw++;
			else
				nufoot++;
			break;

		case '$':
			resync();
			break;

		case ' ':
		case '+':
			if ( message_count < 1 ) {
				current = -1;
				break;
			}

			header_page++;
			nuhead++;

			if ( move_when_paged && header_page <=
			    ((message_count - 1) / headers_per_page) ) {
				current = header_page * headers_per_page + 1;
				if ( selected )
					current = visible_to_index(current) + 1;
			}
			break;

		case '-':
			if ( message_count < 1 ){
				current = -1;
				break;
			}

			header_page--;
			nuhead++;
			if ( move_when_paged && header_page >= 0 ) {
				current = header_page * headers_per_page + 1;
				if ( selected )
					current = visible_to_index( current ) + 1;
			}
			break;

		case '=':
			if ( message_count < 1 ){
				current = -1;
				break;
			}

			if ( selected )
				current = visible_to_index( 1 ) + 1;
			else
				current = 1;

			if ( get_page(current) )
				nuhead++;
			else
				nucurr++;
			break;

		case '*':
			if ( message_count < 1 ){
				current = -1;
				break;
			}


			if ( selected )
				current = visible_to_index( selected ) + 1;
			else
				current = message_count;

			if ( get_page(current) )
				nuhead++;
			else
				nucurr++;
			break;

		case '|':
			if ( message_count < 1 ){
				error( catgets(nl_fd,NL_SETN,52, "No mail to pipe!") );
				fflush( stdout );
				break;
			}

			Writechar( '|' );

			redraw = do_pipe();

			break;

		case '!':
			Writechar( '!' );

			redraw = subshell();

			break;

		case '%':
			if ( message_count < 1 ){
				error( catgets(nl_fd,NL_SETN,13, "Mailbox is empty!") );
				fflush( stdout );
			} else {
				get_return( address, FALSE );
				clear_error();
				PutLine1( LINES, (int)((COLUMNS - strlen(address)) / 2),
				  	 "%.78s", address );
			}

			break;

		case '/':
			if ( message_count < 1 ){
				error( catgets(nl_fd,NL_SETN,13, "Mailbox is empty!") );
				fflush( stdout );
				break;
			}

#ifdef SIGWINCH
			resized = 0;
#endif
			if ( pattern_match() ) {
				if ( get_page(current) )
					nuhead++;
				else
					nucurr++;
			} else {
				error( catgets(nl_fd,NL_SETN,5,"pattern not found!") );
				fflush( stdout );
			}
#ifdef SIGWINCH
			if (resized) {
				resized = 0;
				redraw++;
				nuhead  = 0;
				nucurr = 0;
			} else {
#endif
				ClearLine( LINES - 1 );
				ClearLine( LINES - 2 );
#ifdef SIGWINCH
			}
#endif

			break;

		case '<':	/* scan current message for calendar
				 * information */


#ifdef ENABLE_CALENDAR
			if ( message_count < 1 ) {
				error( catgets(nl_fd,NL_SETN,13, "Mailbox is empty!") );
				fflush( stdout );
				break;
			}

				
			num_char = strlen(catgets(nl_fd,NL_SETN,3,"Command: ") );
			PutLine0( LINES - 3, num_char, catgets(nl_fd,NL_SETN,6,"Scan message for calendar entries..."));
			scan_calendar();
#else
			error( catgets(nl_fd,NL_SETN,7,"Sorry - calendar function disabled") );
			fflush( stdout );
#endif


			break;

		case 'a':
			alias();
			nufoot++;

			if ( hp_terminal )
				define_softkeys( MAIN );
			break;

		case 'b':
#ifdef SIGWINCH
			resized = 0;
#endif
			num_char = strlen(catgets(nl_fd,NL_SETN,3,"Command: "));
			PutLine0( LINES - 3, num_char, catgets(nl_fd,NL_SETN,8,"Bounce message") );
			fflush( stdout );

			if ( message_count < 1 ) {
				error( catgets(nl_fd,NL_SETN,9,"No mail to bounce!") );
				fflush( stdout );
			} else {
				remail();
#ifdef SIGWINCH
				if (resized) {
					redraw++;
					resized = 0;
				} else
#endif
					nufoot++; 
			}

			if ( hp_terminal )
				define_softkeys( MAIN );
			break;

		case 'c':
			num_char = strlen(catgets(nl_fd,NL_SETN,3,"Command: "));
			PutLine0( LINES - 3, num_char, catgets(nl_fd,NL_SETN,10,"Change mailbox") );

			if ( (file_changed = leave_mbox(FALSE)) != -1 ) {
			
				if ( selected )
					selected = 0;

				if ( hp_terminal )
					define_softkeys( CHANGE );

				if ( (redraw = newmbox(0, TRUE, TRUE)) == 2 ) {

					newmbox(1, FALSE, TRUE);

					if ( saved_at_cancel > 0 ) {
						if ( deleted_at_cancel > 0 )
							sprintf( buff,catgets(nl_fd,NL_SETN,54, "[%d message(s) %s %s, and %d deleted]"), saved_at_cancel , keep_in_incoming ? "kept" : "stored", keep_in_incoming ? "" : "in your mbox", deleted_at_cancel );
						else if ( ! keep_in_incoming )
							sprintf( buff, catgets(nl_fd,NL_SETN,55, "[message(s) stored in your mbox]") );
					} else if ( deleted_at_cancel > 0 )
						sprintf( buff, catgets(nl_fd,NL_SETN,56, "[all messages deleted]") );
					else 
						buff[0] = '\0';

					error( buff );

				}

			} else {
				file_changed = 0;
				sort_mailbox( message_count, 0, FALSE );
			}

			if ( hp_terminal )
				define_softkeys( MAIN );
			break;

		case '^':
		case 'd':
			if ( message_count < 1 ) {
				error( catgets(nl_fd,NL_SETN,11,"No mail to delete!") );
				fflush( stdout );
			} else {
				delete_msg( (ch == 'd'), TRUE );

				if ( resolve_mode && next_visible(current) != -1 ) {
					move_to_next();

					if ( get_page(current) )
						nuhead++;
					else
						nucurr++;
				}
			}

			break;

		case ctrl('D'):
#ifdef SIGWINCH
			resized = 0;
#endif
			if ( message_count < 1 ) {
				error( catgets(nl_fd,NL_SETN,11,"No mail to delete!") );
				fflush( stdout );
			} else
				meta_match( DELETED );
#ifdef SIGWINCH
			if (resized) {
				redraw++;
				resized = 0;
			}
#endif
			break;

/*
 * Edit message function is not supported in standard elm.
 */
#ifdef ALLOW_MAILBOX_EDITING
  		case 'e':
			num_char = strlen(catgets(nl_fd,NL_SETN,3,"Command: "));
  			PutLine0( LINES - 3, num_char, catgets(nl_fd,NL_SETN,12,"Edit mailbox") );
  			if ( current > 0 ) {
  				edit_mailbox();
  				if ( has_transmit )
  					transmit_functions( ON );	 * insurance * 
  			} else {
  				error( catgets(nl_fd,NL_SETN,13,"Mailbox is empty!") );
  				fflush( stdout );
  			}
  			break;
/*
 * #else
 *		case 'e':
 *			error( catgets(nl_fd,NL_SETN,14,"mailbox editing isn't configured in this version of Elm"));
 *			break;
 */
#endif


		case 'f':
			num_char = strlen(catgets(nl_fd,NL_SETN,3,"Command: "));
			PutLine0( LINES - 3, num_char, catgets(nl_fd,NL_SETN,15,"Forward") );

			if ( current > 0 ) {

				if ( hp_terminal )
					define_softkeys( YESNO );

				if ( forward() ) {
					ClearScreen();
					update_title();
					last_header_page = -1;
					nuhead++; /*to redraw after   */
					nufoot++; /*checking 'current'*/
				} else {          /* for resolve mode */
					nucurr++;
					nufoot++;
				}

				if ( resolve_mode && next_visible(current) != -1 ) {

					move_to_next();

					if ( get_page(current) ) {
						nuhead++;
						nucurr = 0;
					}
				}

				if ( hp_terminal )
					define_softkeys( MAIN );

			} else {
				error( catgets(nl_fd,NL_SETN,16,"No mail to forward!") );
				fflush( stdout );
			}

			break;

		case 'g':
			num_char = strlen(catgets(nl_fd,NL_SETN,3,"Command: "));
			PutLine0( LINES - 3, num_char, catgets(nl_fd,NL_SETN,17,"Group reply") );
			fflush( stdout );

			if ( current > 0 ) {
	
				if ( header_table[current - 1].status & FORM_LETTER ) {
					error(catgets(nl_fd,NL_SETN,18,"Can't group reply to a Form!!"));
					fflush( stdin );
				} else {
					PutLine0( LINES - 3, COLUMNS - 40,
						  catgets(nl_fd,NL_SETN,19,"building addresses...") );

					if ( hp_terminal )
						define_softkeys( YESNO );

					redraw = reply_to_everyone();

					if ( !redraw && hp_terminal )
						define_softkeys( MAIN );
				}

			} else {
				error( catgets(nl_fd,NL_SETN,20,"No mail to reply to!") );
				fflush( stdout );
			}
			break;

		case 'h':
			if ( filter ) {
				num_char = strlen(catgets(nl_fd,NL_SETN,3,"Command: "));
				PutLine0( LINES - 3, num_char, catgets(nl_fd,NL_SETN,21,"Message with headers...") );
			}
			else {
				num_char = strlen(catgets(nl_fd,NL_SETN,3,"Command: "));
				PutLine0( LINES - 3, num_char, catgets(nl_fd,NL_SETN,22,"Read Message") );
			}

			fflush( stdout );

			i = filter;
			filter = FALSE;

			if ( (redraw = show_msg(current)) > ' '
			    	&& redraw < 127 ) {
				while ( (redraw = process_showmsg_cmd(redraw)) > ' ' )
					 /* just keep callin' that routine */ ;
			}

			redraw++;		/* always.. */
			get_page( current );

			filter = i;
			break;

		case 'j':
			if ( message_count < 1 ) {
				current = -1;
				break;
			}

			move_to_next();

			if ( get_page(current) )
				nuhead++;
			else
				nucurr++;
			break;

		case 'k':
			if ( message_count < 1 ) {
				current = -1;
				break;
			}

			move_to_prev();
			
			if ( current > -1 ) {
				if ( get_page(current) )
					nuhead++;
				else
					nucurr++;
			}

			break;

		case 'l':
			last_selected = selected;
			if ( message_count < 1 ){
				error( catgets(nl_fd,NL_SETN,13, "Mailbox is empty!") );
				fflush( stdout );
				break;
			}

			num_char = strlen(catgets(nl_fd,NL_SETN,3,"Command: "));
			PutLine0( LINES - 3, num_char,
				  catgets(nl_fd,NL_SETN,23,"Limit displayed messages by...") );

#ifdef SIGWINCH
			resized = 0;
#endif
			if ( limit() != 0 && selected != last_selected) {

				if ( selected == 0 )
					last_header_page = -1; 	/* criteria is 'all' */
								/* force redraw	     */
				get_page( current );
#ifdef SIGWINCH
				if (!resized) {
#endif
					nuhead++;
					update_title();
					show_last_error();
#ifdef SIGWINCH
				}
#endif
			} else
				nufoot++;

#ifdef SIGWINCH
			if (resized) {
                                resized = 0;
                                redraw++;
                                nuhead  = 0;
                                nucurr = 0;
                        }
#endif
			if ( hp_terminal )
				define_softkeys( MAIN );

			break;

		case 'm':
			num_char = strlen(catgets(nl_fd,NL_SETN,4,"Command: "));
			PutLine0( LINES - 3, num_char, catgets(nl_fd,NL_SETN,24,"Mail") );

			redraw = send_msg( "", "", "", TRUE, allow_forms, FALSE ) ;

			if ( !redraw && hp_terminal )
				define_softkeys( MAIN );

			break;

		case ctrl('J'):
		case ctrl('M'):
			num_char = strlen(catgets(nl_fd,NL_SETN,4,"Command: "));
			PutLine0( LINES - 3, num_char, catgets(nl_fd,NL_SETN,22,"Read Message") );
			fflush( stdout );

			if ( (redraw = show_msg(current)) > ' '
			               && redraw < 127 ) {

				while ( (redraw = process_showmsg_cmd(redraw)) > ' ' )
					 /* keep calling process-showmsg-cmd */ ;
			}

			redraw++;			/* always.. */

			(void) get_page( current );
			break;

		case 'n':
			num_char = strlen(catgets(nl_fd,NL_SETN,4,"Command: "));
			PutLine0( LINES - 3, num_char, catgets(nl_fd,NL_SETN,26,"Next Message") );
			fflush( stdout );

			move_to_next();

			if ( current <= message_count ){

				if ( (redraw = show_msg(current)) > ' '
			       	        && redraw < 127 ) {

					while ( (redraw=process_showmsg_cmd(redraw)) > ' ' )
						 /* keep circling */ ;
				}

				redraw++;			/* always.. */

				(void) get_page( current );	/* rewrites ANYway */
			}

			break;

		case 'o':
			num_char = strlen(catgets(nl_fd,NL_SETN,4,"Command: "));
			PutLine0( LINES - 3, num_char, catgets(nl_fd,NL_SETN,27, "Options") );

			if ( hp_terminal )
				softkeys_off();

			options();
			redraw++;				/* always fix da screen */
			break;

		case 'p':
			num_char = strlen(catgets(nl_fd,NL_SETN,4,"Command: "));
			PutLine0( LINES - 3, num_char, catgets(nl_fd,NL_SETN,28,"Print mail") );
			fflush( stdout );

			if ( message_count < 1 ) {
				error( catgets(nl_fd,NL_SETN,29,"No mail to print!") );
				fflush( stdout );
			} else
				print_msg();
			break;

		case 'q':
			num_char = strlen(catgets(nl_fd,NL_SETN,4,"Command: "));
			PutLine0( LINES - 3, num_char, catgets(nl_fd,NL_SETN,30,"Quit") );

			if ( mailfile_size < bytes(infile) ) {
				error( catgets(nl_fd,NL_SETN,31,"New Mail!  Quit cancelled...") );
				(void) sleep( 2 );
				fflush( stdout );

				if ( mbox_specified == 0 )
					unlock();
			} else if ( mailfile_size > bytes(infile) ) {
				error( catgets(nl_fd,NL_SETN,33,"Quit cancelled...") );
				(void) sleep( 2 );
				fflush( stdout );

				if ( mbox_specified == 0 )
					unlock();

			} else
				quit();

			break;

		case 'r':
			num_char = strlen(catgets(nl_fd,NL_SETN,4,"Command: "));
			PutLine0( LINES - 3, num_char, catgets(nl_fd,NL_SETN,32,"Reply to message") );

			if ( current > 0 ) {

				redraw = reply();

				if ( !redraw && hp_terminal )
					define_softkeys( MAIN );

			} else {
				error( catgets(nl_fd,NL_SETN,20,"No mail to reply to!") );
				fflush( stdout );
			}

			break;

		case '>':	/** backwards compatibility **/

		case 's':
#ifdef SIGWINCH
			resized = 0;
#endif
			if ( message_count < 1 ) {
				error( catgets(nl_fd,NL_SETN,34,"No mail to save!") );
				fflush( stdout );
			} else {
				num_char = strlen(catgets(nl_fd,NL_SETN,4,"Command: "));
				PutLine0( LINES - 3, num_char, catgets(nl_fd,NL_SETN,35,"Save Message") );
				PutLine0( LINES - 3, COLUMNS - 40,
					  catgets(nl_fd,NL_SETN,36,"(Use '?' to list your folders)") );

				if ( save(&redraw, FALSE) &&
				     resolve_mode && next_visible(current) != -1 ) {

					move_to_next();

					if ( get_page(current) )
						nuhead++;
					else
						nucurr++;
				}
			}

#ifdef SIGWINCH
			if (resized) {
				resized = 0;
				redraw++;
				nucurr = 0;
				nuhead = 0;
				break;
			}
#endif

			if ( !redraw ) {
				show_last_error();

				if ( hp_terminal )
				        define_softkeys( MAIN );
			}

			ClearLine( LINES - 2 );
			break;

		case ctrl('T'):
		case 't':
			if ( message_count < 1 ) {
				error( catgets(nl_fd,NL_SETN,37,"No mail to tag!") );
				fflush( stdout );
			} else if ( ch == 't' ) {
				tag_message( TRUE );
				if ( resolve_mode && next_visible(current) != -1 ) {
					move_to_next();
					if ( get_page(current) )
						nuhead++;
					else
						nucurr++;
				}
			} else
#ifdef SIGWINCH
				{
				resized = 0;
#endif
				meta_match( TAGGED );
#ifdef SIGWINCH
				if (resized) {
					resized = 0;
					redraw++;
				}
			}
#endif

			break;

		case 'u':
			if ( message_count < 1 ) {
				error( catgets(nl_fd,NL_SETN,38,"No mail to mark as undeleted!") );
				fflush( stdout );
			} else {
				undelete_msg( TRUE );

				if ( resolve_mode && next_visible(current) != -1 ) {

					move_to_next();

					if ( get_page(current) )
						nuhead++;
					else
						nucurr++;
				}
			}
			break;

		case ctrl('U'):
#ifdef SIGWINCH
			resized = 0;
#endif
			if ( message_count < 1 ) {
				error( catgets(nl_fd,NL_SETN,39,"No mail to undelete!") );
				fflush( stdout );
			} else
				meta_match( UNDELETE );
#ifdef SIGWINCH
			if (resized) {
				resized = 0;
				redraw++;
			}
#endif
			break;

		case ctrl('Q'):
		case '\x7f':		/* DEL ->  CTRL-? */
		case 'x':
			num_char = strlen(catgets(nl_fd,NL_SETN,4,"Command: "));
			PutLine0( LINES - 3, num_char, catgets(nl_fd,NL_SETN,40,"Exit") );
			fflush( stdout );
			leave( 0 );

		case ctrl('L'):
			redraw++;
			break;

		case '@':
			debug_screen();
			redraw++;
			break;

		case '#':
			if ( message_count ) {
				debug_message();
				redraw++;
			} else {
				error( catgets(nl_fd,NL_SETN,41,"No mail to check") );
				fflush( stdout );
			}
			break;

		case NO_OP_COMMAND:
#ifdef SIGWINCH
			if (resized)
				redraw++;
#endif
			break;

		default:
#ifdef SIGWINCH
			resized = 0;
#endif
			if ( ch > '0' && ch <= '9' ) {
				if ( message_count < 1 ) {
					current = -1;
					break;
				}

				num_char = strlen(catgets(nl_fd,NL_SETN,4,"Command: "));
				PutLine0( LINES - 3, num_char, catgets(nl_fd,NL_SETN,42,"New Current Message") );

				if ( hp_terminal )
					define_softkeys( CANCEL );

				current = read_number( ch );

				if ( hp_terminal )
					define_softkeys( MAIN );

				if ( selected ) {
					if ( (current = visible_to_index(current) + 1) 
					      > message_count )
						goto too_big;
				} else if ( current > message_count )
#ifdef SIGWINCH
									{
					if (resized) {
						resized = 0;
						redraw++;
					}
#endif
					goto too_big;
#ifdef SIGWINCH
				}
#endif

				if ( get_page(current) )
					nuhead++;
				else
					nucurr++;

#ifdef SIGWINCH
				if (resized) {
					resized = 0;
					nucurr = 0;
					nuhead = 0;
					redraw++;
				}
#endif

			} else {
				error( catgets(nl_fd,NL_SETN,43,"Unknown command: Use '?' for commands") );
				fflush( stdin );
			}
			break;
		}

#ifndef SIGWINCH
		if ( redraw )
			showscreen();
#endif

		if ( current < 1 ) {
			if ( message_count > 0 ) {
				error( catgets(nl_fd,NL_SETN,44,"already at the first message!") );
				fflush( stdout );

				current = last_current;

			} else if ( current < 0 ) {
				error( catgets(nl_fd,NL_SETN,45,"No messages to read!") );
				fflush( stdout );
				current = 0;
			}

		} else if ( current > message_count ) {
			if ( message_count > 0 ) {
		too_big:
				if ( selected ) {
					error1( catgets(nl_fd,NL_SETN,46,"only %d message(s) selected!"), 
						selected ); 
					fflush( stdout );
					current = last_current;
				} else {
					error1( catgets(nl_fd,NL_SETN,47,"only %d message(s)!"),
						message_count ); 
					dprint( 8, (debugfile, 
						"[user selected %d, but only %d msgs]\n",
						current, message_count) );
					fflush( stdout );
					
					current = last_current;
				}

			} else {
				error( catgets(nl_fd,NL_SETN,45,"No messages to read!") );
				fflush( stdout );
				current = 0;
			}

		} else if ( selected && (i = visible_to_index(selected)) > message_count ) {
			error1( catgets(nl_fd,NL_SETN,46,"only %d message(s) selected!"), selected );
			fflush( stdout );

			current = last_current;
		}

#ifdef SIGWINCH
		if ( redraw )
			showscreen();
#endif

		if ( nuhead )
			show_headers();

		if ( nucurr )
			show_current();

		if ( nufoot ) {
			if ( mini_menu ) {

				/*
				 * In original source, next line is 
				 * MoveCursor( LINES - 8, 0 ).
				 * But in forwording, cursor position pointer
				 * in cursor.c is changed in somewhere.
				 * So I use this call now.
				 */

				SetCursor( LINES - 8, 0 );
				CleartoEOS();
				show_menu();
				show_last_error();
			} else {

				/*
				 * See the comment above
				 */

				SetCursor( LINES - 4, 0 );
				CleartoEOS();
				show_last_error();
			}
		}
	}					/* the BIG while loop! */
}


int move_to_next()

{
	/*
	 * move the current message pointer to next
	 * This take account of 'skip_deleted', 'selected' flags.
	 */

	if ( skip_deleted && !no_skip_deleted ) {
		if ( selected ) {
			while ( (current = next_visible(current)) > 0 
				 && header_table[current - 1].status & DELETED);

			if ( current < 0 )
				current = message_count + 1;
		} else {
			while ( ++current <= message_count &&
			    (header_table[current - 1].status & DELETED) );
				/** continue moving forward, please **/
		}
	} else {
		if ( selected ) {
			if ( (current = next_visible(current)) < 0 )
		               current = message_count + 1;
		} else
			current++;
	}
}


int move_to_prev()

{
	/*
	 * move the current message pointer to previous one
	 * This take account of 'skip_deleted', 'selected' flags.
	 */

	if ( skip_deleted && !no_skip_deleted ) {
		if ( selected ) {
			while ( (current = previous_visible( current )) > 0 
				 && header_table[current - 1].status & DELETED );
			if ( current < 0 )
				current = -1;
		} else {
			while ( --current > 0 &&
			 	header_table[current - 1].status & DELETED );
				 /* continue searching */ 
		}
	} else {
		if ( selected ) {
			if ( (current = previous_visible( current )) < 0 )
				current = -1;
		} else
			current--;
	}
}


int debug_screen()

{
	/*
	 * spit out all the current variable settings and the table
	 * entries for the current 'n' items displayed. 
	 */


	register int    i, 
			j;
	char            buffer[SLEN];


	ClearScreen();
	Raw( OFF );

	PutLine2( 0, 0, catgets(nl_fd,NL_SETN,48,"Current message number = %d\t\t%d message(s) total\n"),
		  current, message_count );
	PutLine2( 2, 0, catgets(nl_fd,NL_SETN,49,"Header_page = %d           \t\t%d possible page(s)\n"),
		  header_page, (int) (message_count / headers_per_page) + 1 );

	PutLine1( 4, 0, catgets(nl_fd,NL_SETN,50,"\nCurrent mailfile is %s.\n\n"), infile );

	i = header_page * headers_per_page;		/* starting header */

	if ( (j = i + (headers_per_page - 1)) >= message_count )
		j = message_count - 1;

	Write_to_screen( 
		"Num      From                 Subject                         Lines  Offset\n\n", 0, NULL, NULL, NULL);

	while ( i <= j ) {
		sprintf( buffer,
			 "%3d  %-16.16s  %-40.40s  %4d  %d\n",
			 i + 1,
			 header_table[i].from,
			 header_table[i].subject,
			 header_table[i].lines,
			 header_table[i].offset );

		Write_to_screen( buffer, 0, NULL, NULL, NULL );
		i++;
	}

	Raw( ON );

	PutLine0( LINES, 0, catgets(nl_fd,NL_SETN,51,"Press any key to return: ") );
#ifdef SIGWINCH
	while(ReadCh() == NO_OP_COMMAND)
		;
#else
	(void) ReadCh();
#endif
}


int debug_message()

{
	/*
	 * Spit out the current message record.  Include EVERYTHING
	 * in the record structure. 
	 */
	 

	char            buffer[SLEN];


	ClearScreen();
	Raw( OFF );

	Write_to_screen( "\t\t\t----- Message %d -----\n\n\n\n", 1,
			 current, NULL, NULL );

	Write_to_screen( "Lines : %-5d\t\t\tStatus: A  C  D  E  F  N  P  T  U  V\n", 1,
			 header_table[current - 1].lines, NULL, NULL );
	Write_to_screen( "            \t\t\t        c  o  e  x  o  e  r  a  r  i\n", 0,
					NULL, NULL, NULL);
	Write_to_screen( "            \t\t\t        t  n  l  p  r  w  i  g  g  s\n", 0,
					NULL, NULL, NULL);
	Write_to_screen( "            \t\t\t        n  f  d  d  m     v  d  n  i\n", 0,
					NULL, NULL, NULL);

	sprintf( buffer,
		 "\nOffset: %ld\t\t\t        %d  %d  %d  %d  %d  %d  %d  %d  %d  %d\n",
		 header_table[current - 1].offset,
		 (header_table[current - 1].status & ACTION) != 0,
		 (header_table[current - 1].status & CONFIDENTIAL) != 0,
		 (header_table[current - 1].status & DELETED) != 0,
		 (header_table[current - 1].status & EXPIRED) != 0,
		 (header_table[current - 1].status & FORM_LETTER) != 0,
		 (header_table[current - 1].status & NEW) != 0,
		 (header_table[current - 1].status & PRIVATE) != 0,
		 (header_table[current - 1].status & TAGGED) != 0,
		 (header_table[current - 1].status & URGENT) != 0,
		 (header_table[current - 1].status & VISIBLE) != 0 );

	Write_to_screen( buffer, 0, NULL, NULL, NULL );

	sprintf( buffer, "\nReceived on: %d/%d/%d at %d:%02d\n",
		 header_table[current - 1].received.month + 1,
		 header_table[current - 1].received.day,
		 header_table[current - 1].received.year,
		 header_table[current - 1].received.hour,
		 header_table[current - 1].received.minute );

	Write_to_screen( buffer, 0, NULL, NULL, NULL );

	sprintf( buffer, "Message sent on: %s, %s %s, %s at %s\n",
		 header_table[current - 1].dayname,
		 header_table[current - 1].month,
		 header_table[current - 1].day,
		 header_table[current - 1].year,
		 header_table[current - 1].time );

	Write_to_screen( buffer, 0, NULL, NULL, NULL );

	Write_to_screen( "From: %s\nSubject: %s", 2,
			 header_table[current - 1].from,
			 header_table[current - 1].subject, NULL );

	Write_to_screen( "\nPrimary Recipient: %s\nInternal Index Reference Number = %d\n",
		  	 2,
			 header_table[current - 1].to,
			 header_table[current - 1].index_number, NULL );

	Write_to_screen( "Message-ID: %s\n", 1,
			 strlen( header_table[current - 1].messageid ) > 0 ?
			 header_table[current - 1].messageid : "<none>", NULL, NULL);

	Raw( ON );

	PutLine0( LINES, 0, "Please Press any key to return: " );
#ifdef SIGWINCH
	while(ReadCh() == NO_OP_COMMAND)
		;
#else
	(void) ReadCh();
#endif
}
