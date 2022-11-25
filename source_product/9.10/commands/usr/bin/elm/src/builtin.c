/**			builtin.c			**/

/*
 *  @(#) $Revision: 72.3 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  This is the built-in pager for displaying messages while in 
 *  the Elm program.  It's a bare-bones pager with precious few 
 *  options. The idea is that those systems that are sufficiently
 *  slow that using an external pager such as 'more' is too slow,
 *  then they can use this!
 *
 *  Added support for the "builtin+" pager (clears the screen for
 *  each new page) including a two-line overlap for context...
 *
 *  Future add the following functionality;
 *
 *	<number>s	skip <number> lines
 *	<number>f	forward a page or <number> pages
 *
 *  ^L support added nicely...
 */


#include <signal.h>

#include "headers.h"


#define  BEEP		007	/* ASCII Bell character */


#ifdef NLS
# define NL_SETN   6
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


int             lines_put_on_screen = 0,	/* number of lines displayed
						 * per screen                           */
                lines_displayed = 0,		/* Total number of lines displayed      */
                total_lines_to_display,		/* total number of lines in message     */
                pages_displayed,		/* for the nth page titles and all      */
                lines_to_ignore = 0;		/* for 'f' and 's' functions...         */

char            last_line[VERY_LONG_STRING],		/* for displaying an overlap when ..    */
                line_before_that[VERY_LONG_STRING];	/* .. we have a multipage message       */

int 
start_builtin( lines_in_message )

	int             lines_in_message;

{
	/*
	 *  clears the screen and resets the internal counters... 
	 */

	dprint( 8, (debugfile,
		   "displaying %d lines from message using internal pager\n",
		   lines_in_message) );

	lines_displayed = 0;
	lines_put_on_screen = 0;
	lines_to_ignore = 0;
	pages_displayed = 1;
	last_line[0] = '\0';
	line_before_that[0] = '\0';

	total_lines_to_display = lines_in_message;
}


int
display_line(line)

	char           *line;

{
	/*
	 *  Display the given line on the screen, taking into account such
	 *  dumbness as wraparound and such.  If displaying this would put
	 *  us at the end of the screen, put out the "MORE" prompt and wait
	 *  for some input.   Return non-zero if the user terminates the
	 *  paging (e.g. 'q' or 'i') or zero if we should continue...Also, 
         *  this will pass back the value of any character the user types in 
	 *  at the prompt instead, if needed... (e.g. if it can't deal with
	 *  it at this point).
	 */


	register int    lines_needed, 		/*how many line needed*/
			okay_char, 		/*valid command ?     */
			ch, 			/*command char        */
			len, 			/*length of line      */
			formfeed_char = 0;
	int		scroll_up,		/*scrolled up by input*/
			more_lines,		/*more lines          */
			percentage,		/*% of displayed      */
	   		row,			/*current position    */
			col;                    /*current position    */
	char            pattern[SLEN], 		/*search pattern      */
			display_buffer[SLEN], 	/*more message buffer */
			*p, 
			*pend;


	if ( lines_to_ignore != 0 ) {
		if ( --lines_to_ignore <= 0 ) {
			(void) putchar( '\n' );
			lines_to_ignore = 0;
		}

		return ( 0 );
	}

	lines_needed = (int) (printable_chars((unsigned char *)line) / COLUMNS);

	/*
	 * the preceeding is to deal with wraparound 
	 */

	pend = line + strlen( line );

	for ( p = line; p < pend; )
		if ( *p++ == '\n' )
			lines_needed++;

	if ( (formfeed_char = line[0] == '\f') ||
	     lines_needed + lines_put_on_screen > LINES - 1 ) {
		more_lines = total_lines_to_display - lines_displayed;
		percentage = (int) ( 100.0 * (1.0 * lines_displayed) /
			     (1.0 * total_lines_to_display) + 0.5 );
		if ( user_level == 0 && percentage < 100 )
			sprintf( display_buffer,
			catgets(nl_fd,NL_SETN,1," %d line(s) left (%d%%) : press <space> for more, or 'q' to return "),
				 more_lines, percentage );

		else if ( user_level == 0 )
			sprintf( display_buffer,
			catgets(nl_fd,NL_SETN,1," %d line(s) left : press <space> for more, or 'q' to return "),
				 more_lines );

		else if ( user_level == 1 && percentage < 100 )
			sprintf( display_buffer,
			catgets(nl_fd,NL_SETN,2," %d line(s) more (%d%%) : Press <space> for more, 'q' to return "),
				 more_lines, percentage );

		else if ( user_level == 1 )
			sprintf( display_buffer,
			catgets(nl_fd,NL_SETN,2," %d line(s) more : Press <space> for more, 'q' to return "),
				 more_lines );

  		else if ( percentage < 100 )
			sprintf( display_buffer,
			catgets(nl_fd,NL_SETN,3," %d line(s) more (you've seen %d%%) "),
				 more_lines, percentage );

  		else
			sprintf( display_buffer, catgets(nl_fd,NL_SETN,3, " %d line(s) more "),
				 more_lines );

		len = strlen( display_buffer );

		StartInverse();
		len += GetXmcStatus();

		if ( formfeed_char ) {
			Write_to_screen( "^L", 0, NULL, NULL, NULL );
			len += 2;
		}

		Write_to_screen( display_buffer, 0, NULL, NULL, NULL );

		EndInverse();
		len += GetXmcStatus();

		okay_char = FALSE;

		do {
			ch=(char)tolower( ReadCh() );

			switch ( ch ) {
#ifndef SIGWINCH
			case '\0':
#endif
			case '\n':
			case '\r':	/* <return> pressed... */
				lines_put_on_screen -= lines_needed;
				okay_char = TRUE;
				break;

			case ' ':	/* <space> pressed... */
				lines_put_on_screen = 0;
				okay_char = TRUE;
				break;

			case '/':       /* search pattern...  */
				Writechar( '/' );
				fflush( stdout );
				pattern[0] = '\0';
				GetXYLocation( &row, &col );
				col += 2*GetXmcStatus();

				optionally_enter( pattern, row, col, FALSE );

				scroll_up=(int)(len+strlen(pattern)+1)/COLUMNS+row-LINES;

				if ( scroll_up < 0 )
					scroll_up = 0;

				SetCursor( row-scroll_up, 0 );
				CleartoEOS();
				Write_to_screen(
					catgets(nl_fd,NL_SETN,4,"...searching for pattern \"%s\"..."),
					1, pattern, NULL, NULL );
				fflush(stdout);

				if ( search( line, pattern ) != TRUE ){
					MoveCursor( row-scroll_up, 0 );
					CleartoEOS();
					StartInverse();
					Write_to_screen(
					catgets(nl_fd,NL_SETN,5,"Pattern not found : Press any key to continue"),
					   0, NULL, NULL, NULL );
					EndInverse();
#ifdef SIGWINCH
					while(ReadCh() == NO_OP_COMMAND)
						;
#else
					(void)ReadCh();
#endif
					MoveCursor( row-scroll_up, 0 );
					CleartoEOS();
					StartInverse();
					Write_to_screen( display_buffer, 0, NULL, NULL, NULL);
					EndInverse();
					fflush( stdout );
					break;

				} else {
					len = strlen(
					catgets(nl_fd,NL_SETN,4,"...searching for pattern \"%s\"...")) 
						+ strlen( pattern ) - 2;
					okay_char = TRUE;
					break;
				}

			case 'i':
			case 'q':
			case 'Q':
				return (TRUE);	/* get OUTTA here! */

#ifdef SIGWINCH
			case (char)NO_OP_COMMAND:
				break;
#endif

			default:
				dprint( 2, (debugfile,
					   "builtin is outta here, returning '%c' [%d]\n",
					   ch, ch) );
				return ( ch );
			}

		} while ( !okay_char );

		if ( clear_pages && lines_put_on_screen == 0 ) {	/* not for <ret> */
			ClearScreen();

			if ( title_messages && filter ) {
				title_for_page( ++pages_displayed );
				display_line( "\n" );	/* blank line for fun */
			}

			if ( strlen(line_before_that) > 0 ) {
				display_line( line_before_that );
				display_line( last_line );
			} else if ( strlen(last_line) > 0 )
				display_line( last_line );

		} else {
		 	CursorLeft( len );		/* back up to the beginning
						 	 * of line */
			CleartoEOLN();
			if ( len >= COLUMNS ) {
				CursorLeft( COLUMNS );
				CursorUp( 1 );
				CleartoEOLN();
			}
		} 

	} else if ( lines_needed + lines_put_on_screen > LINES - 5 ) {
		strcpy( line_before_that, last_line );
		if ( formfeed_char ) {
			strcpy( last_line, "^L" );
			strcat( last_line, (char *) line + 1 );
		} else
			strcpy( last_line, line );
	}
	if ( formfeed_char ) {
		Write_to_screen( "^L", 0, NULL, NULL, NULL );
		Write_to_screen( "%s", 1, (char *) line + 1, NULL, NULL );
	} else
		Write_to_screen( "%s", 1, line, NULL, NULL );

	lines_put_on_screen += lines_needed;		/* read from file   */

	return ( FALSE );
}


int 
title_for_page(page)

	int             page;

{
	/*
	 *  output a nice title for the second thru last pages of the message 
	 *  we're currently reading... 
	 */


	char            *p, 
			title[SLEN], 
			title2[SLEN], 
			from_buffer[50], 
			from_buffer2[50];
	register int    padding, sl, mail_sent;


	strncpy( from_buffer, header_table[current - 1].from, 50 );

	tail_of( from_buffer, from_buffer2, TRUE );

	mail_sent = ( first_word(header_table[current - 1].from, "To:") );

	sprintf( title, "%s #%d %s %s",
		header_table[current - 1].status & FORM_LETTER ? "Form" : "Message",
		current,
		mail_sent ? "to" : "from", from_buffer2	);

	padding = COLUMNS - (sl = strlen(title))
		  - (page < 10 ? 7 : page < 100 ? 8 : 9);

	p = title + sl;

	while ( padding-- > 0 )
		*p++ = ' ';

	*p = '\0';

	sprintf( title2, " Page %d\n", page );

	strcat( title, title2 );

	display_line( title );
}


int
search( in_buffer, pat )

	char		*in_buffer,
	                *pat;

{
	/*
	 * This routine searches the given pattern in the message.
	 * The search is done from current line to the end.
	 * If can't find the pattern, then return FALSE, and
	 * if get it, then return TRUE.
	 * The line that includes the pattern is located at the
	 * center of screen.
	 */


	char            buffer[VERY_LONG_STRING];
	int		lines; 
	register int	line;


	if ( in_string( in_buffer, pat ) ){
		lines_put_on_screen = (int)LINES/2;
		feed_lines();
		return( TRUE );
	}

	line = lines_displayed;			/*search from this line*/
	lines = header_table[current-1].lines;  /*search till this line*/

	while ( fgets(buffer, VERY_LONG_STRING, mailfile) != NULL && line < lines ) {

		line++;

		if ( in_string(buffer, pat) ) {	/*let's search      */
			if ( line <= lines_displayed + LINES/2 ){
				lines_put_on_screen = LINES/2 - (line-lines_displayed);
				feed_lines();
				return( TRUE );

			} else {
				lines_displayed = line - LINES/2;
				lines_put_on_screen = 0;
				feed_lines();
				return( TRUE );
			}
		}
	}

	feed_lines();				/* fault to find, reset pointer */
	return( FALSE );
}


int
feed_lines()

{
	/*
	 * This routine advance the pointer for display
	 * message after search.
	 */


	register int	skip;
	char	 	buffer[VERY_LONG_STRING];


	skip = lines_displayed+1;
	fseek( mailfile, header_table[current-1].offset, 0L);
	while( skip-- )
		fgets( buffer, VERY_LONG_STRING, mailfile );
	return( 0 );
}
