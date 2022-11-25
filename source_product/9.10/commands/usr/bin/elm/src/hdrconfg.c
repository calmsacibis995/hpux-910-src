/**			hdrconfg.c			**/

/*
 *  @(#) $Revision: 70.8 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *    This file contains the routines necessary to be able to modify
 *    the mail headers of messages on the way off the machine.  The
 *    headers currently supported for modification are:
 *
 *	Subject:
 *	To:
 *	Cc:
 *	Bcc:
 *	Reply-To:
 *
 *	The next set are a function of whether we're running in X.400 
 *	compatability mode or not...If we aren't, then we have:
 *
 *	   Expires:
 *	   Priority:
 *         In-Reply-To:
 *	   Action:
 *
 *	  <user defined>
 *
 *	Otherwise we have:
 *
 *	   Expires:
 *	   Importance:
 *	   Level-of-Sensitivity:	(really "Sensitivity")
 *	   Notification:
 *
 */

#include <signal.h>

#include "headers.h"


#ifdef NLS
# define NL_SETN  19
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


/*
 * these are all defined in the mailmsg file! 
 */

extern char     subject[SLEN], 
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

extern int	in_header_editor,
		no_to_header;

extern int	num_char;
#ifdef ALLOW_BCC
extern char     bcc[VERY_LONG_STRING], 
		expanded_bcc[VERY_LONG_STRING];
#endif


#ifdef ENABLE_NAMESERVER
extern int      screen_changed;
#endif

#ifdef SIGWINCH
extern int    resized;        /* flag indicating window resize occurred */
#endif


edit_headers()

{
	/*
	 *  Edit headers.  
	 */


#ifndef ENABLE_NAMESERVER
	int             unexpanded_to = TRUE, 
			unexpanded_cc = TRUE,
			unexpanded_reply_to = TRUE;

# ifdef ALLOW_BCC
	int             unexpanded_bcc = TRUE;
# endif

#endif


	int             c,
			i = 0;
	register char	address1[VERY_LONG_STRING],
			comment[LONG_SLEN];


	if ( strlen(to) > VERY_LONG_STRING )
		unexpanded_to = FALSE;

	if ( strlen(cc) > VERY_LONG_STRING )
		unexpanded_cc = FALSE;

	if ( strlen(bcc) > VERY_LONG_STRING )
		unexpanded_bcc = FALSE;

	if ( strlen(reply_to) > VERY_LONG_STRING )
		unexpanded_reply_to = FALSE;
	
/*
 *  In replying, the address has the style such as
 *  addrees (comment), so remove this comment for editing.
 */

	expanded_to[0] = '\0';	
	if ( in_string(to, "(" ) ) {
		while ( break_down_tolist(to, &i, address1, comment) ) {
			strcat( expanded_to, address1 );
			strcat( expanded_to, ", " );
		}
		expanded_to[strlen(expanded_to) -2] = '\0';
		strcpy( to, expanded_to );
		expanded_to[0] = '\0';	
	}

	if ( mail_only )
		goto outta_here;		/* how did we get HERE??? */

	display_headers();
	clearerr( stdin );

#ifdef SIGWINCH
	resized = 0;
#endif
	while ( TRUE ) {			/* forever */
#ifdef SIGWINCH
redraw:
		if (resized) {
			resized = 0;
			display_headers();
		}
#endif
		PutLine0( LINES - 1, 0, catgets(nl_fd,NL_SETN,1,"Choice: ") );
		CleartoEOLN();
#ifdef SIGWINCH
		while((c = toupper(getchar())) == EOF) {
			if (feof(stdin))
				break;
			clearerr(stdin);
			goto redraw;
		}
#else
		c = toupper(getchar());
#endif

		if ( c == EOF )
			break;

		clear_error();

		switch ( c ) {
		case RETURN:
		case LINE_FEED:
		case 'Q':
			goto outta_here;

		case ctrl('L'):
			display_headers();
			break;

		case 'T':
			in_header_editor = TH;

			if ( strlen( to ) > VERY_LONG_STRING )
				to[VERY_LONG_STRING - 1] = '\0';

			/* subtract 2 for the "%s"; minor kludge to avoid adding a new msg */
			num_char = strlen(catgets(nl_fd,NL_SETN,24,"To : %s")) - 2;

			if ( optionally_enter(to, 2, num_char, TRUE) == -1 )
				goto outta_here;


#ifdef ENABLE_NAMESERVER
			if ( nameserver[0] != '\0' )
				PutLine0( LINES - 2, 8, catgets(nl_fd,NL_SETN,2," (checking) ") );

			/*
			 *  screen-changed is a global variable set by the
 	 		 *  return value of the nameserver if we actually
 			 *  go ahead and call one ...
 		         */

			if ( build_address(to, expanded_to) && !screen_changed )
				PutLine1( 2, num_char, "%s", to );
#else
			if ( unexpanded_to ) {
				(void) build_address( to, expanded_to );
				unexpanded_to = FALSE;
			}

			if ( strlen(to) > 2*COLUMNS - 5 )
				display_headers();
#endif


			break;

		case 'S':
			in_header_editor = JH;
			PutLine0( 8, 0, catgets(nl_fd,NL_SETN,17,"Subject: "));
			num_char = strlen(catgets(nl_fd,NL_SETN,17,"Subject: "));
			if ( optionally_enter(subject, 8, num_char, FALSE) == -1 )
				goto outta_here;
			break;


#ifdef ALLOW_BCC
		case 'B':
			in_header_editor = BH;
			PutLine0( 5, 0, catgets(nl_fd,NL_SETN,18,"Bcc: "));
			num_char = strlen(catgets(nl_fd,NL_SETN,18,"Bcc: "));
			if ( optionally_enter(bcc, 5, num_char, TRUE) == -1 )
				goto outta_here;

#ifdef ENABLE_NAMESERVER

			if ( nameserver[0] != '\0' )
				PutLine0( LINES - 2, 8, catgets(nl_fd,NL_SETN,2," (checking) ") );

			/*
			 *  screen-changed is a global variable set by the
 	 		 *  return value of the nameserver if we actually
 			 *  go ahead and call one ...
 		         */

			if ( build_address(bcc, expanded_bcc) && !screen_changed )
				PutLine1( 5, 5, "%s", bcc );
#else
			if ( unexpanded_bcc ) {
				(void) build_address( bcc, expanded_bcc );
				unexpanded_bcc = FALSE;
			}

			if ( strlen(bcc) > COLUMNS - 5 )
				display_headers();
#endif

			if ( strlen(expanded_to) + strlen(expanded_cc)
			    +strlen(expanded_bcc) > VERY_LONG_STRING ) {
				error( catgets(nl_fd,NL_SETN,15,"Too many people. Blind Copies ignored !" ));
				bcc[0] = '\0';
				expanded_bcc[0] = '\0';
			}

			break;
#endif


		case 'C':
			in_header_editor = CH;
			PutLine0( 4, 0, catgets(nl_fd,NL_SETN,19,"Cc: "));
			if ( strlen( cc ) > VERY_LONG_STRING )
				cc[VERY_LONG_STRING- 1] = '\0';

			num_char = strlen(catgets(nl_fd,NL_SETN,19,"Cc: "));

			if ( optionally_enter(cc, 4, num_char, TRUE) == -1 )
				goto outta_here;


#ifdef ENABLE_NAMESERVER
			if ( nameserver[0] != '\0' )
				PutLine0( LINES - 2, 8, catgets(nl_fd,NL_SETN,2," (checking) ") );

			/*
			 *  screen-changed is a global variable set by the
 	 		 *  return value of the nameserver if we actually
 			 *  go ahead and call one ...
 		         */

			if ( build_address(cc, expanded_cc) && !screen_changed )
				PutLine1( 4, 5, "%s", cc );
#else
			if ( unexpanded_cc ) {
				(void) build_address( cc, expanded_cc );
				unexpanded_cc = FALSE;
			}

			if ( strlen(cc) > COLUMNS - 5 )
				display_headers();
#endif

			if ( strlen(expanded_to) + strlen(expanded_cc)
			    +strlen(expanded_bcc) > VERY_LONG_STRING ) {
				error( catgets(nl_fd,NL_SETN,16,"Too many people. Copies ignored !" ));
				cc[0] = '\0';
				expanded_cc[0] = '\0';
			}

			break;

		case 'R':
			in_header_editor = RH;
			PutLine0( 6, 0, catgets(nl_fd,NL_SETN,20,"Reply-To: "));
			num_char = strlen(catgets(nl_fd,NL_SETN,20,"Reply-To: "));
			if ( optionally_enter(reply_to, 6, num_char, TRUE) == -1 )
				goto outta_here;
			if ( unexpanded_reply_to ) {
				(void) build_address( reply_to, expanded_reply_to );
				unexpanded_reply_to = FALSE;
			}
			if ( strlen(reply_to) > 2*COLUMNS - 10 )
				display_headers();

			break;

		case 'E':
			PutLine1( 11, 0, catgets(nl_fd,NL_SETN,30,"Expires: %s"), expires);
			CleartoEOLN();
			/* subtract 2 for the "%s"; minor kludge to avoid adding a new msg */
			num_char = strlen(catgets(nl_fd,NL_SETN,30,"Expires: %s")) - 2;
			enter_date( 11, num_char, expires );
			break;

		case 'A':
			in_header_editor = AH;
			PutLine0( 10, 0, catgets(nl_fd,NL_SETN,21,"Action: "));
			num_char = strlen(catgets(nl_fd,NL_SETN,21,"Action: "));
			if ( optionally_enter(action, 10, num_char, FALSE) == -1 )
				goto outta_here;
			if ( strlen(action) > COLUMNS - 9 )
				display_headers();
			break;

		case 'P':
			in_header_editor = PH;
			PutLine0( 12, 0, catgets(nl_fd,NL_SETN,22,"Priority: "));
			num_char = strlen(catgets(nl_fd,NL_SETN,22,"Priority: "));
			if ( optionally_enter(priority, 12, num_char, FALSE) == -1 )
				goto outta_here;
			if ( strlen(priority) > COLUMNS - 10 && strlen(in_reply_to) > 0 )
				display_headers();
			break;

		case 'U':
			in_header_editor = UH;
			MoveCursor( 15, 0 );
			CleartoEOLN();
			if ( optionally_enter(user_defined_header, 15, 0, FALSE) == -1 )
				goto outta_here;
			else
				check_user_header( user_defined_header );
			break;

		case 'I':
			in_header_editor = IH;
			if ( strlen(in_reply_to) > 0 ) {
				PutLine0( 13, 0, catgets(nl_fd,NL_SETN,23,"In-Reply-To: "));
				num_char = strlen(catgets(nl_fd,NL_SETN,23,"In-Reply-To: "));
				if ( optionally_enter(in_reply_to, 13, num_char, FALSE) == -1 )
					goto outta_here;
				break;
			}

			/*
			 *  else fall through as an error 
			 */

		default:
			Centerline( LINES, catgets(nl_fd,NL_SETN,5,"Unknown header being specified!") );
		}


#ifdef ENABLE_NAMESERVER
		if ( screen_changed )
			display_headers();
#endif


	}

outta_here:			/*
				 * this section re-expands aliases before we
				 * leave... 
				 */

	in_header_editor = FALSE;
	if ( strlen(to) == 0 )
		no_to_header = TRUE;
#ifndef ENABLE_NAMESERVER
	if ( unexpanded_to )
		(void) build_address( to, expanded_to );
	if ( unexpanded_cc )
		(void) build_address( cc, expanded_cc );
	if ( unexpanded_reply_to )
		(void) build_address( reply_to, expanded_reply_to );

# ifdef ALLOW_BCC
	if ( unexpanded_bcc )
		(void) build_address( bcc, expanded_bcc );
# endif

#else

	/*
	 * a no-op so the goto has somewhere to go to 
	 */

	;

#endif

}


int
display_headers()

{
	ClearScreen();

	Centerline( 0, catgets(nl_fd,NL_SETN,6,"Message Header Edit Screen") );

	PutLine1( 2, 0, catgets(nl_fd,NL_SETN,24,"To : %s"), to );
	if ( strlen(to)+5 > 2*COLUMNS ) {
		MoveCursor( 4, 0 );
		CleartoEOS();
	}
			
	PutLine1( 4, 0, catgets(nl_fd,NL_SETN,25,"Cc : %s"), cc );
	if ( strlen(cc)+5 > COLUMNS ) {
		MoveCursor( 5, 0 );
		CleartoEOS();
	}


#ifdef ALLOW_BCC
	PutLine1( 5, 0, catgets(nl_fd,NL_SETN,26,"Bcc: %s"), bcc );
	if ( strlen(bcc)+5 > COLUMNS ) {
		MoveCursor( 6, 0 );
		CleartoEOS();
	}
#endif

	PutLine1( 6, 0, catgets(nl_fd,NL_SETN,27,"Reply-To: %s"), reply_to);
	if ( strlen(reply_to)+10 > 2*COLUMNS ) {
		MoveCursor( 8, 0 );
		CleartoEOS();
	}

	PutLine1( 8, 0, catgets(nl_fd,NL_SETN,28,"Subject: %s"), subject );
	if ( strlen(subject)+9 > 2*COLUMNS ) {
		MoveCursor( 10, 0 );
		CleartoEOS();
	}

	PutLine1( 10, 0, catgets(nl_fd,NL_SETN,29,"Action: %s"), action );
	if ( strlen(action)+9 > COLUMNS ) {
		MoveCursor( 11, 0 );
		CleartoEOS();
	}

	PutLine1( 11, 0, catgets(nl_fd,NL_SETN,30,"Expires: %s"),expires );
	PutLine1( 12, 0, catgets(nl_fd,NL_SETN,31,"Priority: %s"),priority);

	if ( strlen(in_reply_to) > 0 ) {
		if ( strlen(priority)+10 > COLUMNS ) {
			MoveCursor( 13, 0 );
			CleartoEOS();
		}

		PutLine1( 13, 0, catgets(nl_fd,NL_SETN,23,"In-Reply-To: %s"),in_reply_to);
	}

	if ( strlen(user_defined_header) > 0 ) {
		if ( strlen(in_reply_to)+13 > 2*COLUMNS ) {
			MoveCursor( 15, 0 );
			CleartoEOS();
		}

		PutLine1( 15, 0, "%s", user_defined_header );
	}

	Centerline( LINES - 5,
		    catgets(nl_fd,NL_SETN,7,"Choose first letter of existing header, U)ser defined header, or <return>") );


#ifdef ENABLE_NAMESERVER
	screen_changed = 0;			/* obviously it's okay here! */
#endif
	if ( in_header_editor > 0 ) {
		in_header_editor = TRUE;
		PutLine0( LINES - 1, 0, catgets(nl_fd,NL_SETN,1,"Choice: ") );
	}
}


int 
enter_date( x, y, buffer )

	int             x, 
			y;
	char           *buffer;

{
	/*
	 *  Enter the number of days this message is valid for, then
	 *  display at (x,y) the actual date of expiration.  This 
	 *  routine relies heavily on the routine 'days_ahead()' in
	 *  the file date.c
	 */


	int             days;


	PutLine0( LINES - 2, 0,
		  catgets(nl_fd,NL_SETN,9,"How many days in the future should this message expire? ") );
	CleartoEOLN();
	Raw( OFF );
	gets( buffer );
	Raw( ON );
	sscanf( buffer, "%d", &days );

	if ( days < 1 ) {
		Centerline( LINES, catgets(nl_fd,NL_SETN,10,"That doesn't make sense!") );
		(void) sleep( 3 );
	} else if ( days > 56 ) {
		Centerline( LINES,
		     catgets(nl_fd,NL_SETN,11,"Expiration date must be within eight weeks of today") );
		(void) sleep( 3 );
	} else {
		days_ahead( days, buffer );
		PutLine0( x, y, buffer );

	}

	MoveCursor( LINES - 2, 0 );
	CleartoEOLN();
}


int 
check_user_header( header )

	char           *header;

{
	/*
	 *  check the header format...if bad print error and erase! 
	 */

	register int    i = -1;

	if ( strlen(header) == 0 )
		return;

	if ( whitespace(header[0]) ) {
		error( catgets(nl_fd,NL_SETN,12,"you can't have leading white space in a header!") );
		header[0] = '\0';
		ClearLine( 14 );
		return;
	}
	
	if ( header[0] == ':' ) {
		error( catgets(nl_fd,NL_SETN,13,"you can't have a colon as the first character!") );
		header[0] = '\0';
		ClearLine( 14 );
		return;
	}

	while ( header[++i] != ':' ) {
		if ( header[i] == '\0' ) {
			error( catgets(nl_fd,NL_SETN,14,"You need to have a colon ending the field name!") );
			header[0] = '\0';
			ClearLine( 14 );
			return;
		} else if ( whitespace(header[i]) ) {
			error( catgets(nl_fd,NL_SETN,15,"You can't have white space embedded in the header name!"));
			header[0] = '\0';
			ClearLine( 14 );
			return;
		}
	}

	return;
}
