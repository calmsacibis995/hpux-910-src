/**			in_utils.c			**/

/*
 *  @(#) $Revision: 70.3 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  Mindless I/O routines for ELM 
 */


#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <termio.h>

#include "headers.h"

#ifdef NLS
# include <nl_ctype.h>
/* share msg catalog with hdrconfg.c */
# define NL_SETN  19
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


extern int      errno;		/* system error number */

extern int	in_header_editor;

extern char	subject[SLEN],
		in_reply_to[SLEN],
		expires[SLEN],
		action[SLEN],
		priority[SLEN],
		reply_to[VERY_LONG_STRING],
		to[VERY_LONG_STRING],
		cc[VERY_LONG_STRING],
		user_defined_header[SLEN];

#ifdef ALLOW_BCC
extern char	bcc[VERY_LONG_STRING];
#endif

unsigned long   alarm();


#define isstopchar(c)		(c == ' ' || c == '\t' || c == '/')
#define isslash(c)		(c == '/')
#define erase_a_char()		{ Writechar(BACKSPACE); Writechar(' '); \
			          Writechar(BACKSPACE); fflush(stdout); }

int
want_to( question, dflt, echo_answer )

	char            *question;
	char            dflt;
	int             echo_answer;

{
	/*
	 *  Ask 'question' at LINES-2, COLUMNS-40, returning the answer in 
	 *  lower case.  If 'echo_answer', then echo answer.  'dflt' is the 
	 *  default answer if <return> is pressed. (Note: 'dflt' is also what 
	 *  will be returned if <return> is pressed!)
	 */


	register char   ch;
	int 		cols;


	if ( question != NULL ) {
		cols = ( strlen(question) < 30) ? COLUMNS - 40 : COLUMNS - 50;
		PutLine3( LINES - 3, cols, "%s%c%c", question, dflt, BACKSPACE );
		fflush( stdout );
	}

	fflush( stdin );

	ch = yesno( dflt, echo_answer );

	return ( ch );
}


int
yesno( dflt, echo_answer )

	char            dflt;
	int             echo_answer;

{
	/*
	 *  get a 'y' or an 'n' -- ignore all other input 
	 */


	char            ch;


	fflush( stdin );

	do {
		ch = (char)tolower( ReadCh() );

		switch ( ch ) {
		case '\r':
		case '\n':
			ch = dflt;
			break;
		}

	} while ( (ch != 'y') && (ch != 'n') && (ch != eof_char) );

	if ( echo_answer && ch != eof_char )
		printf( "%s", ch == 'y' ? "Yes" : "No" );
	else if ( echo_answer )
		printf( "Cancelled" );

	return ( ch );
}


int
read_number( ch )

	char            ch;

{
	/*
	 *  Read a number, where 'ch' is the leading digit! 
	 */


	char            buff[SLEN];
	int             num;


	buff[0] = ch;
	buff[1] = '\0';

/*	PutLine0( LINES - 3, COLUMNS - 40, "Set current message to :" ); */
    PutLine0( LINES - 3, COLUMNS -40, catgets(nl_fd,NL_SETN,32,"Set current message to : ") );

	if ( optionally_enter( buff, LINES - 3, COLUMNS - 15, TRUE) )
		return ( current );

	sscanf( buff, "%d", &num );
	return ( num );
}


int
optionally_enter( string, x, y, append_current )

	char           *string;
	int             x, 
			y, 
			append_current;

{
	/*
	 *  This will display the string on the screen and allow the user to
	 *  either accept it (by pressing return) or alter it according to
	 *  what the user types.   The various flags are:
	 *       string    is the buffer to use (with optional initial value)
	 *	 x,y	   is the location we're at on the screen (-1,-1 means
	 *		   that we can't use this info and need to explicitly
	 *		   use backspace-space-backspace sequences)
	 *	 append_current  means that we have an initial string and that
	 *		   the cursor should be placed at the END of the line,
	 *		   not the beginning (the default).
	 *
	 *  If we hit an interrupt or EOF we'll return non-zero.
	 */


	int             ch,
			escape = FALSE,
			columns = COLUMNS -1,
			scroll = 0,			/* for backspace at 0 column */
			max_index = 0,			/* to calculate scroll lines */
			start_row,
			end_row,
			initial_len = 0;		/* initial value length */

	register int    ch_count = 0, 
			row,
			index = 0, 
			is_tab = 0,
			use_cursor_control;

#ifdef SIGWINCH
	struct winsize	w_before, w_after;
	void		(*wstat) ();
#endif

	struct { int	w_ch_flag;			/* for NLS if 2-byte char ? */
		 int	screen_col;			/* position of the char     */
	       }	buf_flag[VERY_LONG_STRING];			/* for backspace sequence   */

	
	/*
	 * at first, initialize the flags 
	 */

	for ( index=0; index<VERY_LONG_STRING; index++ ) {
		buf_flag[index].w_ch_flag = 0;		/* this used only in NLS    */
		buf_flag[index].screen_col = -1;	/* used always for BS       */
	}

	index = 0;

	clearerr( stdin );

	if ( (use_cursor_control = (!mail_only && x >= 0 && y >= 0)) )
		PutLine1( x, y, "%s", string );
	else
		printf( "%s", string );

	CleartoEOLN();

	if ( !append_current ) {
		if ( use_cursor_control ){

			SetCursor( x, y );

			if ( y >= columns )
				buf_flag[index].screen_col = columns;
			else
				buf_flag[index].screen_col = y;

		} else
			non_destructive_back_up( strlen(string) );

	} else {
		initial_len = strlen( string );


#ifdef NLS
		/*
		 * set the flags for existing value 
		 * the flags means:
		 * buf_flag[index].w_ch_flag  ->  0: 1-byte character
		 *                                1: 1st char of 2-byte char
		 *                                2: 2nd char of 2-byte char
		 * buf_flag[index].screen_col ->  cursor position of the char
		 */

		for( index=0; index<initial_len; index++ ){

			ch = *(string + index);

			if ( index > 0 ){

				if ( buf_flag[index-1].w_ch_flag == 1 ){
					if ( SECof2(ch) )
						buf_flag[index].w_ch_flag = 2;
					else{
						buf_flag[index-1].w_ch_flag = 0;
						buf_flag[index].w_ch_flag = 0;
					}
				} else {
					if ( FIRSTof2(ch) )
						buf_flag[index].w_ch_flag = 1;
					else
						buf_flag[index].w_ch_flag = 0;
				}

				if ( use_cursor_control ){

					if ( buf_flag[index-1].screen_col == columns
				   	   || (buf_flag[index-1].screen_col == columns -1 
					   && buf_flag[index].w_ch_flag == 1) )
						buf_flag[index].screen_col = 0;
					else
						buf_flag[index].screen_col
					 	  = buf_flag[index-1].screen_col + 1;

				}

			} else {
				if ( use_cursor_control )
					if ( y >= columns )
						buf_flag[index].screen_col = columns;
					else
						buf_flag[index].screen_col = y;

				if ( FIRSTof2(ch) ){
					buf_flag[index].w_ch_flag = 1;
					if (use_cursor_control && y >= columns)
						buf_flag[index].screen_col = 0;
				} else
					buf_flag[index].w_ch_flag = 0;
			}
		}
#endif NLS


		index = initial_len;
	}

	if ( has_transmit )
		transmit_functions( OFF );

#ifdef SIGWINCH
	/* ignore SIGWINCH while string is being input.  check window
	 * size before and after and reset screen size if necessary.
	 */

	 ioctl(0, TIOCGWINSZ, &w_before);
	 wstat = signal(SIGWINCH, SIG_IGN);
#endif

	/*
	 *  now we have the screen as we want it and the cursor in the 
	 *  right place, we can loop around on the input and return the
	 *  string as soon as the user presses <return>
	 */

	do {
		ch = getchar();

		if ( ch == TAB ) {
			is_tab = 8;
			ch = ' ';
		}

		if ( ch == eof_char ) {		/* we've hit EOF */
			string[initial_len] = '\0';

			if ( has_transmit )
				transmit_functions( ON );

#ifdef SIGWINCH
			/* see if window was resized */
			ioctl(0, TIOCGWINSZ, &w_after);
			if ((w_before.ws_row != w_after.ws_row) ||
					(w_before.ws_col != w_after.ws_col))
				setsize(w_after.ws_row, w_after.ws_col);
			signal(SIGWINCH, wstat);
#endif

			return ( 1 );
		}

		if ( ch_count++ == 0 ) {
			if ( ch == EOF || ch == '\n' || ch == '\r' ) {
				if ( has_transmit )
					transmit_functions( ON );
#ifdef SIGWINCH
				ioctl(0, TIOCGWINSZ, &w_after);
				if ((w_before.ws_row != w_after.ws_row) ||
					(w_before.ws_col != w_after.ws_col))
					setsize(w_after.ws_row, w_after.ws_col);
				signal(SIGWINCH, wstat);
#endif
				return ( 0 );
			} else if ( !append_current ) {
				CleartoEOLN();
				index = ( append_current ? strlen(string) : 0 );
			}
		}

		/*
		 * the following is converted from a case statement to allow
		 * the variable characters (backspace, kill_line and break)
		 * to be processed.  Case statements in C require constants
		 * as labels, so it failed ... 
		 */

		if ( ch == backspace ) {

			if ( escape ) {
				CleartoEOLN();
				escape = FALSE;
				continue;
			}

			if ( index > 0 ) {


#ifdef NLS
				if ( use_cursor_control
				     && (buf_flag[index-1].screen_col == columns
				        || (buf_flag[index-1].screen_col == columns -1
					    && buf_flag[index-1].w_ch_flag == 2)) ){

					scroll = x + (y + max_index)/COLUMNS - LINES;
					if (scroll < 0)
						scroll = 0;
					
					SetCursor( x-scroll+index/COLUMNS,
					  	   buf_flag[index-1].screen_col ); 

					if ( buf_flag[index-1].w_ch_flag == 2 ){
						Writechar( ' ' );
						SetCursor( x-scroll+index/COLUMNS,
					  	  buf_flag[index-1].screen_col ); 
						index--;
						erase_a_char();
						index--;
					} else {
						Writechar( ' ' );
						SetCursor( x-scroll+index/COLUMNS,
					  	  buf_flag[index-1].screen_col ); 
						index--;
					}

				} else {
					if ( buf_flag[index-1].w_ch_flag == 2 ){
						index--;
						erase_a_char();
						Writechar( BACKSPACE );
						index--;

					} else {
						Writechar( BACKSPACE );
						index--;
					}

					Writechar( ' ' );
					Writechar( BACKSPACE );

					if ( use_cursor_control
					     && buf_flag[index].screen_col == 0
					     && buf_flag[index-1].screen_col == columns-1 ){
						scroll = x + (y + max_index)/COLUMNS - LINES;
						if ( scroll < 0 )
							scroll = 0;
					
						SetCursor( x-scroll+index/COLUMNS,
				  		           columns ); 
					}

				}

			} else {
				Writechar( ' ' );
				Writechar( BACKSPACE );
			}
#else NLS

				if ( buf_flag[index].screen_col == 0
				     && use_cursor_control ){

					scroll = x + (y + max_index)/COLUMNS - LINES;

					if ( scroll < 0 )
						scroll = 0;
					
					SetCursor( x-scroll+index/COLUMNS,
					   buf_flag[index-1].screen_col ); 

					Writechar( ' ' );

					SetCursor( x-scroll+index/COLUMNS,
					  buf_flag[index-1].screen_col ); 
				} else 
					erase_a_char();	

				index--;

			} else {
				Writechar( ' ' );
				Writechar( BACKSPACE );
			}
#endif NLS
			fflush( stdout );

		} else if ( ch == EOF || ch == '\n' || ch == '\r' ) {
			string[index] = '\0';
			if ( has_transmit )
				transmit_functions( ON );
#ifdef SIGWINCH
			ioctl(0, TIOCGWINSZ, &w_after);
			if ((w_before.ws_row != w_after.ws_row) ||
				(w_before.ws_col != w_after.ws_col))
				setsize(w_after.ws_row, w_after.ws_col);
			signal(SIGWINCH, wstat);
#endif
			return ( 0 );

		} else if ( ch == ctrl('W') ) {	/* back up a word! */
			if ( index == 0 )
				continue;	/* no point staying here.. */
			index--;

			if ( use_cursor_control ){
				scroll = x + (y + max_index)/COLUMNS - LINES;

				if ( scroll < 0 ){
					scroll = 0;
					end_row = x + (y + max_index)/COLUMNS;
				} else
					end_row = LINES;
			}

			if ( isslash(string[index]) ) {
					if ( ! use_cursor_control )
						erase_a_char();
			} else {
				while ( index >= 0 && isspace(string[index]) ) {
					index--;
					if ( ! use_cursor_control )
						erase_a_char();
				}

				while ( index >= 0 && !isstopchar(string[index]) ) {
					index--;
					if ( ! use_cursor_control )
						erase_a_char();
				}

				index++;	/* and make sure we point at
						 * the first AVAILABLE slot */
			}

			if ( use_cursor_control ){
				start_row = x - scroll + (y+index)/COLUMNS;

				for ( row = end_row; start_row < row; row-- ){
					SetCursor( row, 0 );
					CleartoEOLN();
				}

				MoveCursor( start_row, buf_flag[index].screen_col );
				CleartoEOLN();
			}

		} else if ( ch == ctrl('R') ) {
			string[index] = '\0';

			if ( use_cursor_control ) {
				scroll = x + (y + max_index)/COLUMNS - LINES;

				if ( scroll < 0 )
					scroll = 0;
			
				PutLine1( x-scroll, y, "%s", string ); 
				CleartoEOS();
			} else
				printf( "\n\r%s", string );

		} else if ( ch == kill_line && escape == FALSE ) {
			if ( use_cursor_control ) {
				scroll = x + (y + max_index)/COLUMNS - LINES;

				if ( scroll < 0 )
					scroll = 0;
			
				SetCursor( x-scroll, y ); 
				CleartoEOS();
				if ( in_header_editor > 0 ) {
					switch( in_header_editor ) {
					case TH:
						to[0] = '\0';
						break;
#ifdef ALLOW_BCC
					case BH:
						bcc[0] = '\0';
						break;
#endif
					case CH:
						cc[0] = '\0';
						break;
					case JH:
						subject[0] = '\0';
						break;
					case AH:
						action[0] = '\0';
						break;
					case IH:
						in_reply_to[0] = '\0';
						break;
					case RH:
						reply_to[0] = '\0';
						break;
					case PH:
						priority[0] = '\0';
						break;
					case UH:
						user_defined_header[0] = '\0';
						break;
					}
					display_headers();
					SetCursor( x-scroll, y ); 
				}
			} else
				back_up( index + 1);

			index = 0;

		} else if ( ch == '\0' ) {
			if ( has_transmit )
				transmit_functions( ON );
			fflush( stdin );	/* remove extraneous chars, if any */
			string[0] = '\0';	/* clean up string, and... */
#ifdef SIGWINCH
			ioctl(0, TIOCGWINSZ, &w_after);
			if ((w_before.ws_row != w_after.ws_row) ||
				(w_before.ws_col != w_after.ws_col))
				setsize(w_after.ws_row, w_after.ws_col);
			signal(SIGWINCH, wstat);
#endif
			return ( -1 );

#ifdef NLS
		} else if ( !(isprint(ch) || FIRSTof2(ch) || SECof2(ch)) ) {
#else

		} else if ( !isprint(ch) ) {

#endif

			putchar( (char) 007 );	/* BEEP ! */
			continue;

		} else do {	/* default case */


#ifdef NLS
			string[index] = ch;

			if ( index > 0 ){

				if ( buf_flag[index-1].w_ch_flag == 1 ){

					if ( SECof2(ch) )
						buf_flag[index].w_ch_flag = 2;
					else{
						buf_flag[index-1].w_ch_flag = 0;
						buf_flag[index].w_ch_flag = 0;
					}

				} else {

					if ( FIRSTof2(ch) )
						buf_flag[index].w_ch_flag = 1;
					else
						buf_flag[index].w_ch_flag = 0;
				}

				if ( use_cursor_control ) {
				     if ( (buf_flag[index-1].screen_col == columns -1
				             && buf_flag[index].w_ch_flag == 1)
				          || buf_flag[index-1].screen_col == columns )
					buf_flag[index].screen_col = 0;
				     else
					buf_flag[index].screen_col = buf_flag[index-1].screen_col +1;
				}

			} else {
				if ( use_cursor_control ){
					if ( y >= columns )
						buf_flag[index].screen_col = columns;
					else
						buf_flag[index].screen_col = y;
				}

				if ( FIRSTof2(ch) ){
					buf_flag[index].w_ch_flag = 1;

					if ( use_cursor_control && y >= columns )
						buf_flag[index].screen_col = 0;
						
				} else
					buf_flag[index].w_ch_flag = 0;
				
			}

			Writechar( ch );

			if ( ch == '\134' && escape == FALSE ) {
				max_index = max_index<(index+1) ? index+1:max_index;
				if ( use_cursor_control &&
				     buf_flag[index].screen_col == columns){
					scroll = x + (y + max_index)/COLUMNS - LINES;

					if ( scroll < 0 )
						scroll = 0;
				
					SetCursor( x-scroll+(index+1)/COLUMNS,
			  		           columns ); 
				} else
					Writechar( BACKSPACE );
				escape = TRUE;
			} else {
				escape = FALSE;
				index++;
			}

			if ( index == VERY_LONG_STRING ){
				putchar( (char) 007 );	/* BEEP ! */

				if ( ! use_cursor_control
				     || (use_cursor_control 
				 	&& buf_flag[index-1].screen_col != columns) ){

					if ( buf_flag[index-1].w_ch_flag == 2 ){
						index--;
						erase_a_char();
						Writechar( BACKSPACE );
						index--;
					} else {
						Writechar( BACKSPACE );
						index--;
					}
					Writechar( ' ' );
					Writechar( BACKSPACE );

					if ( use_cursor_control
					     && buf_flag[index].screen_col == 0
					     && buf_flag[index-1].screen_col == columns-1 ){
						scroll = x + (y + max_index)/COLUMNS - LINES;
						if ( scroll < 0 )
							scroll = 0;
				
						SetCursor( x-scroll+index/COLUMNS,
				  		           columns); 
					}
				}

				if ( use_cursor_control 
				     && buf_flag[index-1].screen_col == columns ){

					scroll = x + (y + max_index)/COLUMNS - LINES;

					if ( scroll < 0 )
						scroll = 0;
					
					SetCursor( x-scroll+index/COLUMNS,
					  buf_flag[index-1].screen_col ); 

					if ( buf_flag[index-1].w_ch_flag == 2 ){
						Writechar( ' ' );
						SetCursor( x-scroll+index/COLUMNS,
					  	  buf_flag[index-1].screen_col ); 
						index--;
						erase_a_char();
						index--;
					} else {
						Writechar( ' ' );
						SetCursor( x-scroll+index/COLUMNS,
					  	  buf_flag[index-1].screen_col ); 
						index--;
					}
				}
			}
#else NLS
			if ( use_cursor_control )
				if ( index > 0 ){
					if ( buf_flag[index-1].screen_col == columns )
						buf_flag[index].screen_col = 0;
					else
						buf_flag[index].screen_col = buf_flag[index-1].screen_col +1;
				} else
					buf_flag[index].screen_col = y;

			string[index] = ch;
			Writechar( ch );

			if ( use_cursor_control && ch == '\\' && escape == FALSE ) {
				max_index = max_index<(index+1) ? index+1:max_index;

				if ( buf_flag[index].screen_col == columns ){
					scroll = x + (y + max_index)/COLUMNS - LINES;

					if ( scroll < 0 )
						scroll = 0;
				
					SetCursor( x-scroll+(index+1)/COLUMNS,
			  		           columns ); 
				} else
					Writechar( BACKSPACE );

				escape = TRUE;
			} else {
				escape = FALSE;
				index++;
			}

			if ( index == VERY_LONG_STRING ){
				putchar( (char) 007 );	/* BEEP ! */

				if ( ! use_cursor_control ){
					Writechar( BACKSPACE );
					Writechar( ' ' );
					Writechar( BACKSPACE );
				} else if ( buf_flag[index-1].screen_col == columns ){
					scroll = x + (y + max_index)/COLUMNS - LINES;

					if ( scroll < 0 )
						scroll = 0;
				
					SetCursor( x-scroll+index/COLUMNS,
					   buf_flag[index-1].screen_col ); 

					Writechar( ' ' );

					SetCursor( x-scroll+index/COLUMNS,
					  buf_flag[index-1].screen_col ); 
				} else 
					erase_a_char();

				index--;
			}
#endif NLS

			if ( is_tab )
				is_tab--;


		} while ( is_tab );

		if ( use_cursor_control )
			max_index = max_index<index ? index:max_index;

		is_tab = 0;			/* ensure reset it */

	} while ( index < VERY_LONG_STRING );

#ifdef SIGWINCH
	ioctl(0, TIOCGWINSZ, &w_after);
	if ((w_before.ws_row != w_after.ws_row) ||
			(w_before.ws_col != w_after.ws_col))
		setsize(w_after.ws_row, w_after.ws_col);
	signal(SIGWINCH, wstat);
#endif
}


int
pattern_enter( string, alt_string, x, y, alternate_prompt )

	char            *string, 
			*alt_string, 
			*alternate_prompt;
	int             x, 
			y;
{
	/*
	 *  This function is functionally similar to the routine
	 *  optionally-enter, but if the first character pressed
	 *  is a '/' character, then the alternate prompt and string
	 *  are used rather than the normal one.  This routine 
	 *  returns 1 if alternate was used, 0 if not, -1 if cancelled.
	 */


	int		columns = COLUMNS - 1,
			scroll = 0,
			escape = FALSE,
			max_index = 0,
			start_row,
#ifdef SIGWINCH
			rc,
#endif
			end_row;

	char            ch;
	register        index = 0,
			is_tab = 0,
			row;

#ifdef SIGWINCH
	struct winsize	w_before, w_after;
	void		(*wstat) ();
#endif

	struct { int	w_ch_flag;
		 int	screen_col;
	       }	buf_flag[SLEN];


	for ( index = 0; index < SLEN; index++ ){
		buf_flag[index].w_ch_flag = 0;
		buf_flag[index].screen_col = -1;
	}

	index = 0;

	PutLine1( x, y, "%s", string );
	CleartoEOLN();
	MoveCursor( x, y );

	if ( has_transmit )
		transmit_functions( OFF );

#ifdef SIGWINCH
	ioctl(0, TIOCGWINSZ, &w_before);
	wstat = signal(SIGWINCH, SIG_IGN);
#endif

	do {
		if ( index++ )
			putchar( (char) 007 );	/* BEEP ! */

		ch = getchar();

	} while ( ch == ESCAPE );

	index = 0;				/* reset it */

	if ( ch == '\n' || ch == '\r' || ch == eof_char ) {
		if ( has_transmit )
			transmit_functions( ON );
#ifdef SIGWINCH
		ioctl(0, TIOCGWINSZ, &w_after);
		if ((w_before.ws_row != w_after.ws_row) ||
				(w_before.ws_col != w_after.ws_col))
			setsize(w_after.ws_row, w_after.ws_col);
		signal(SIGWINCH, wstat);
#endif
		return ( -1 );			/* we're done.  No change needed */
	}

	if ( ch == '/' ) {
		PutLine1( x, 0, "%s", alternate_prompt );
		CleartoEOLN();
#ifdef SIGWINCH
		rc = optionally_enter(alt_string, x, strlen(alternate_prompt)+1,
					 FALSE);
		ioctl(0, TIOCGWINSZ, &w_after);
		if ((w_before.ws_row != w_after.ws_row) ||
				(w_before.ws_col != w_after.ws_col))
			setsize(w_after.ws_row, w_after.ws_col);
		signal(SIGWINCH, wstat);
		if (rc)
#else
		if (optionally_enter(alt_string, x, strlen(alternate_prompt)+1,
					FALSE))
#endif
			return(-1);
		else
			return(1);
	}

	MoveCursor( LINES - 3, 40 );
	CleartoEOLN();
	MoveCursor( x, y );
	CleartoEOLN();

	index = 0;

	if ( ch == kill_line ) {
		MoveCursor( x, y );
		CleartoEOLN();
		index = 0;

	} else if ( ch != backspace && isalnum((int)ch) ) {

#ifdef NLS
		if ( y >= columns )
			buf_flag[index].screen_col = columns;
		else
			buf_flag[index].screen_col = y;

		if ( FIRSTof2(ch) ){
			buf_flag[index].w_ch_flag = 1;

			if ( y >= columns )
				buf_flag[index].screen_col = 0;
				
		} else
			buf_flag[index].w_ch_flag = 0;
#else NLS
		buf_flag[index].screen_col = y;
#endif NLS

		Writechar( ch );
		string[index++] = ch;

	} else {
		Writechar( ' ' );
		Writechar( BACKSPACE );
	}

	do {

		fflush( stdout );
		ch = getchar();

		if ( ch == ESCAPE ) {
			putchar( (char) 007 );	/* BEEP ! */
			continue;
		}

		if ( ch == TAB ) {		/* put TAB into 8 spaces */
			is_tab = 8;
			ch = ' ';
		}

		if ( ch == eof_char ) {

			if ( has_transmit )
				transmit_functions( ON );
			
#ifdef SIGWINCH
			ioctl(0, TIOCGWINSZ, &w_after);
			if ((w_before.ws_row != w_after.ws_row) ||
					(w_before.ws_col != w_after.ws_col))
				setsize(w_after.ws_row, w_after.ws_col);
			signal(SIGWINCH, wstat);
#endif
			return( -1 );
		}

		/*
		 * the following is converted from a case statement to allow
		 * the variable characters (backspace, kill_line and break)
		 * to be processed.  Case statements in C require constants
		 * as labels, so it failed ... 
		 */

		if ( ch == backspace ) {

			if ( escape ) {
				CleartoEOLN();
				escape = FALSE;
				continue;
			}

			if ( index > 0 ) {


#ifdef NLS
				if ( buf_flag[index-1].screen_col == columns
				        || (buf_flag[index-1].screen_col == columns -1
					    && buf_flag[index-1].w_ch_flag == 2) ){

					scroll = x + (y + max_index)/COLUMNS - LINES;

					if ( scroll < 0 )
						scroll = 0;
					
					SetCursor( x-scroll+index/COLUMNS,
					  buf_flag[index-1].screen_col ); 

					if ( buf_flag[index-1].w_ch_flag == 2 ){
						Writechar( ' ' );
						SetCursor( x-scroll+index/COLUMNS,
					  	  buf_flag[index-1].screen_col ); 
						index--;
						erase_a_char();
						index--;
					} else {
						Writechar( ' ' );
						SetCursor( x-scroll+index/COLUMNS,
					  	  buf_flag[index-1].screen_col ); 
						index--;
					}

				} else {
					if ( buf_flag[index-1].w_ch_flag == 2 ){
						index--;
						erase_a_char();
						Writechar( BACKSPACE );
						index--;

					} else {
						Writechar( BACKSPACE );
						index--;
					}

					Writechar( ' ' );
					Writechar( BACKSPACE );

					if ( buf_flag[index].screen_col == 0
					     && buf_flag[index-1].screen_col == columns-1 ){
						scroll = x + (y + max_index)/COLUMNS - LINES;
						if ( scroll < 0 )
							scroll = 0;
					
						SetCursor( x-scroll+index/COLUMNS,
				  		           columns ); 
					}

				}

			} else {
				Writechar( ' ' );
				Writechar( BACKSPACE );
			}
#else NLS
				if ( buf_flag[index].screen_col == 0 ){

					scroll = x + (y + max_index)/COLUMNS - LINES;

					if ( scroll < 0 )
						scroll = 0;
					
					SetCursor( x-scroll+index/COLUMNS,
					   buf_flag[index-1].screen_col) ; 

					Writechar( ' ' );

					SetCursor( x-scroll+index/COLUMNS,
					  buf_flag[index-1].screen_col ); 
				} else 
					erase_a_char();	

				index--;

			} else {
				Writechar( ' ' );
				Writechar( BACKSPACE );
			}
#endif NLS


			fflush( stdout );

		} else if ( ch == EOF || ch == '\n' || ch == '\r' ) {
			string[index] = '\0';

			if ( has_transmit )
				transmit_functions( ON );
#ifdef SIGWINCH
			ioctl(0, TIOCGWINSZ, &w_after);
			if ((w_before.ws_row != w_after.ws_row) ||
					(w_before.ws_col != w_after.ws_col))
				setsize(w_after.ws_row, w_after.ws_col);
			signal(SIGWINCH, wstat);
#endif
			return ( 0 );

		} else if ( ch == ctrl('W') ) {		/* back up a word! */
			if ( index == 0 )
				continue;		/* no point staying here.. */
			index--;

			scroll = x + (y + max_index)/COLUMNS - LINES;

			if ( scroll < 0 ){
				scroll = 0;
				end_row = x + (y + max_index)/COLUMNS;
			} else
				end_row = LINES;

			if ( isslash(string[index]) ) {
			} else {
				while ( index >= 0 && isspace(string[index]) ) {
					index--;
				}

				while ( index >= 0 && !isstopchar(string[index]) ) {
					index--;
				}

				index++;	/* and make sure we point at
						 * the first AVAILABLE slot */
			}

			start_row = x - scroll + (y+index)/COLUMNS;

			for ( row = end_row; start_row < row; row-- ){
				SetCursor( row, 0 );
				CleartoEOLN();
			}

			MoveCursor( start_row, buf_flag[index].screen_col );
			CleartoEOLN();

		} else if ( ch == ctrl('R') ) {
			string[index] = '\0';
			scroll = x + (y + max_index)/COLUMNS - LINES;

			if ( scroll < 0 )
				scroll = 0;
		
			PutLine1( x-scroll, y, "%s", string ); 
			CleartoEOLN();

		} else if ( ch == kill_line && escape == FALSE ) {
			scroll = x + (y + max_index)/COLUMNS - LINES;

			if ( scroll < 0 )
				scroll = 0;
		
			SetCursor( x-scroll, y ); 
			CleartoEOS();
			index = 0;

		} else if ( ch == '\0' ) {
			if ( has_transmit  )
				transmit_functions( ON );
			fflush( stdin );		/*remove extraneous chars, if any*/
			string[0] = '\0';		/* clean up string, and... */
#ifdef SIGWINCH
			ioctl(0, TIOCGWINSZ, &w_after);
			if ((w_before.ws_row != w_after.ws_row) ||
					(w_before.ws_col != w_after.ws_col))
				setsize(w_after.ws_row, w_after.ws_col);
			signal(SIGWINCH, wstat);
#endif
			return ( -1 );

		} else do {				/* default case */


#ifdef NLS
			string[index] = ch;

			if ( index > 0 ){

				if ( buf_flag[index-1].w_ch_flag == 1 ){

					if ( SECof2(ch) )
						buf_flag[index].w_ch_flag = 2;
					else{
						buf_flag[index-1].w_ch_flag = 0;
						buf_flag[index].w_ch_flag = 0;
					}
				} else {

					if ( FIRSTof2(ch) )
						buf_flag[index].w_ch_flag = 1;
					else
						buf_flag[index].w_ch_flag = 0;
				}

				if ( (buf_flag[index-1].screen_col == columns -1
				             && buf_flag[index].w_ch_flag == 1)
				          || buf_flag[index-1].screen_col == columns )
					buf_flag[index].screen_col = 0;
				else
					buf_flag[index].screen_col 
							= buf_flag[index-1].screen_col +1;

			} else {
				if ( y >= columns )
					buf_flag[index].screen_col = columns;
				else
					buf_flag[index].screen_col = y;

				if ( FIRSTof2(ch) ){
					buf_flag[index].w_ch_flag = 1;

					if ( y >= columns )
						buf_flag[index].screen_col = 0;
						
				} else
					buf_flag[index].w_ch_flag = 0;
				
			}

			Writechar( ch );
			if ( ch == '\134' && escape == FALSE ) {
				max_index = max_index<(index+1) ? index+1:max_index;

				if ( buf_flag[index].screen_col == columns ){
					scroll = x + (y + max_index)/COLUMNS - LINES;

					if ( scroll < 0 )
						scroll = 0;
				
					SetCursor( x-scroll+(index+1)/COLUMNS,
			  		           columns ); 
				} else
					Writechar( BACKSPACE );

				escape = TRUE;
			} else {
				escape = FALSE;
				index++;
			}

			if ( index == SLEN ){
				putchar( (char) 007 );	/* BEEP ! */

				if ( buf_flag[index-1].screen_col != columns ){

					if ( buf_flag[index-1].w_ch_flag == 2 ){
						index--;
						erase_a_char();
						Writechar( BACKSPACE );
						index--;
					} else {
						Writechar( BACKSPACE );
						index--;
					}

					Writechar( ' ' );
					Writechar( BACKSPACE );

					if ( buf_flag[index].screen_col == 0
					     && buf_flag[index-1].screen_col == columns-1 ){
						scroll = x + (y + max_index)/COLUMNS - LINES;
						if ( scroll < 0 )
							scroll = 0;
				
						SetCursor( x-scroll+index/COLUMNS,
				  		           columns ); 
					}
				}

				if ( buf_flag[index-1].screen_col == columns ){

					scroll = x + (y + max_index)/COLUMNS - LINES;

					if ( scroll < 0 )
						scroll = 0;
					
					SetCursor( x-scroll+index/COLUMNS,
					  buf_flag[index-1].screen_col ); 

					if ( buf_flag[index-1].w_ch_flag == 2 ){
						Writechar( ' ' );
						SetCursor( x-scroll+index/COLUMNS,
					  	  buf_flag[index-1].screen_col ); 
						index--;
						erase_a_char();
						index--;
					} else {
						Writechar( ' ' );
						SetCursor( x-scroll+index/COLUMNS,
					  	  buf_flag[index-1].screen_col ); 
						index--;
					}
				}
			}

#else NLS
			if ( index > 0 ){
				if ( buf_flag[index-1].screen_col == columns )
					buf_flag[index].screen_col = 0;
				else
					buf_flag[index].screen_col = buf_flag[index-1].screen_col +1;
			} else
				buf_flag[index].screen_col = y;

			string[index] = ch;
			Writechar( ch );

			if ( ch == '\\' && escape == FALSE ) {
				max_index = max_index<(index+1) ? index+1:max_index;
				if ( buf_flag[index].screen_col == columns ){
					scroll = x + (y + max_index)/COLUMNS - LINES;

					if ( scroll < 0 )
						scroll = 0;
				
					SetCursor( x-scroll+(index+1)/COLUMNS,
			  		           columns ); 
				} else
					Writechar( BACKSPACE );

				escape = TRUE;
			} else {
				escape = FALSE;
				index++;
			}

			if ( index == SLEN ){
				putchar( (char) 007 );	/* BEEP ! */

				if ( buf_flag[index-1].screen_col == columns ){
					scroll = x + (y + max_index)/COLUMNS - LINES;

					if ( scroll < 0 )
						scroll = 0;
				
					SetCursor( x-scroll+index/COLUMNS,
					   buf_flag[index-1].screen_col ); 

					Writechar( ' ' );

					SetCursor( x-scroll+index/COLUMNS,
					  buf_flag[index-1].screen_col ); 
				} else 
					erase_a_char();

				index--;
			}
#endif NLS

			if ( is_tab )
				is_tab--;

		} while ( is_tab );

		is_tab = 0;			/* ensure reset */

		max_index = max_index<index ? index:max_index;

	} while ( index < SLEN );

#ifdef SIGWINCH
	ioctl(0, TIOCGWINSZ, &w_after);
	if ((w_before.ws_row != w_after.ws_row) ||
			(w_before.ws_col != w_after.ws_col))
		setsize(w_after.ws_row, w_after.ws_col);
	signal(SIGWINCH, wstat);
#endif
}


int
back_up( spaces )

	int             spaces;

{
	/*
	 *  this routine is to replace the goto x,y call for when sending
	 *  mail without starting the entire "elm" system up... 
	 */


	while ( spaces-- ) {
		erase_a_char();
	}
}


int 
non_destructive_back_up( spaces )

	int             spaces;

{
	/*
	 *  same as back_up() but doesn't ERASE the characters on the screen 
	 */

	while ( spaces-- )
		Writechar( BACKSPACE );

	fflush( stdout );
}


int
GetPrompt()

{
	/*
	 *  This routine does a read/timeout for a single character.
	 *  The way that this is determined is that the routine to
	 *  read a character is called, then the "errno" is checked
	 *  against EINTR (interrupted call).  If they match, this
	 *  returns NO_OP_COMMAND otherwise it returns the normal
	 *  command.	
	 */

	
	register char	ch,		/* command entered */
			lowered;	/* to lowered      */


	if ( timeout > 0 ) {

		(void) alarm( (unsigned long) timeout );

		errno = 0;	/* we actually have to do this.  *sigh*  */

		ch = ReadCh();

		if ( errno == EINTR )
			ch = NO_OP_COMMAND;

		(void) alarm( (unsigned long) 0 );
	} else
		ch = ReadCh();

	if ( ch == 'K' || ch == 'J' )
		no_skip_deleted++;
	else
		no_skip_deleted = 0;
	lowered = (char)tolower( (int)ch );

	return ( lowered );
}


int
remove_spaces( string )

	char           *string;

{
	char           *strtmp;

	while( whitespace( string[strlen(string) - 1] ) )
		string[strlen(string) - 1] = '\0';
	
	strtmp = string;
	while( whitespace( *strtmp ) )
		strtmp++;
	strcpy( string, strtmp );

}
