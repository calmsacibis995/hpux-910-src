/** 			curses.c			**/

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
 *   This library gives programs the ability to easily access the
 *   termcap information and write screen oriented and raw input
 *   programs.  The routines can be called as needed, except that
 *   to use the cursor / screen routines there must be a call to
 *   InitScreen() first.  The 'Raw' input routine can be used
 *   independently, however.
 * 
 *
 *  NOTE THE ADDITION OF: the #ifndef ELM stuff around routines that
 *  we don't use.  This is for code size and compile time speed...
 */


#include "headers.h"

#ifdef RAWMODE
# include <termio.h>
# include <time.h>
# include <signal.h>
# include <setjmp.h>
# define TTYIN	0
#endif

#ifdef SHORTNAMES
# define _clearinverse	_clrinv
# define _cleartoeoln	_clrtoeoln
# define _cleartoeos	_clr2eos
# define _transmit_off	xmit_off
# define _transmit_on	xmit_on
#endif

#ifdef RAWMODE
struct termio   _raw_tty, 
		_original_tty;
static int      _inraw = 0;	/* are we IN rawmode?    */
#endif


/* we're back to level zero, I think.. */

#define DEFAULT_LINES_ON_TERMINAL	24
#define DEFAULT_COLUMNS_ON_TERMINAL	80

#ifndef ELM
static int      _memory_locked = 0;	/* are we IN memlock??   */
#endif

static int      _line = -1,		/* initialize to "trash" */
                _col = -1;

static int	_magiccookie;		/* have blank char at enhance */
static int      _intransmit;		/* are we transmitting keys? */

static
char            *_clearscreen, 
		*_moveto, 
		*_up, 
		*_down, 
		*_right, 
		*_left, 
		*_setinverse, 
		*_clearinverse, 
		*_cleartoeoln, 
		*_cleartoeos, 
		*_transmit_on, 
		*_transmit_off;

static
struct {	
	char	*escape_seq,
		mapped[2];
       } 	keypad_map[10];

static
int             _lines, 
		_columns;

static 
char     	_terminal[1024],	/* Storage for terminal entry */
		_capabilities[1024],	/* String for cursor motion   */
		*ptr = _capabilities;	/* for buffering              */

int             outchar();		/* char output for tputs      */

void		trapalarm();

extern int	tgetent(), tgetnum(), tgetflag, tputs();
extern char	*tgetstr(), *tgoto();

int 
InitScreen()

{
	/*
	 * Set up all this fun stuff: returns zero if all okay, or; -1
	 * indicating no terminal name associated with this shell, -2..-n  No
	 * termcap for this terminal type known 
	 */


	int             err;
	char		*dflt_tab_char;
	char            termname[40],
			tmptermname[40];


	if ( getenv("TERM") == NULL )
		return ( -1 );

	if ( strcpy(termname, getenv("TERM")) == NULL )
		return ( -1 );

	if ( !first_word(termname, "hp") ){
		strcpy( tmptermname, "hp" );
		strcat( tmptermname, termname );
		if ( tgetent(_terminal, tmptermname) != 1 )
			hp_device = FALSE;
		else
			hp_device = TRUE;
	} else 
		hp_device = TRUE; 

	if ( (err = tgetent(_terminal, termname)) != 1 )
		return ( err - 2 );

	_line = 0;			/* where are we right now?? */
	_col = 0;			/* assume zero, zero...     */
	_magiccookie = 0;		/* has no "xmc" (initialize)*/

	/*
	 * load in all those pesky values 
	 */

	_clearscreen = tgetstr( "cl", &ptr );
	_moveto = tgetstr( "cm", &ptr );
	_up = tgetstr( "up", &ptr );
	_down = tgetstr( "do", &ptr );
	_right = tgetstr( "nd", &ptr );
	_left = tgetstr( "bs", &ptr );
	_setinverse = tgetstr( "so", &ptr );
	_clearinverse = tgetstr( "se", &ptr );

	/* For keypad mapping */

	keypad_map[1].escape_seq = tgetstr( "kd", &ptr ); /* DOWN */
	keypad_map[1].mapped[0] = 'j';
	keypad_map[1].mapped[1] = '\0';
	keypad_map[2].escape_seq = tgetstr( "kr", &ptr ); /* RIGHT*/
	keypad_map[2].mapped[0] = 'j';
	keypad_map[2].mapped[1] = '\0';
	keypad_map[3].escape_seq = tgetstr( "kl", &ptr ); /* LEFT */
	keypad_map[3].mapped[0] = 'k';
	keypad_map[3].mapped[1] = '\0';
	keypad_map[4].escape_seq = tgetstr( "kh", &ptr ); /* HOME */
	keypad_map[4].mapped[0] = '=';
	keypad_map[4].mapped[1] = '\0';
	keypad_map[5].escape_seq = tgetstr( "kH", &ptr ); /* DOWN HOME*/
	keypad_map[5].mapped[0] = '*';
	keypad_map[5].mapped[1] = '\0';
	keypad_map[6].escape_seq = tgetstr( "kN", &ptr ); /* NEXT */
	keypad_map[6].mapped[0] = '+';
	keypad_map[6].mapped[1] = '\0';
	keypad_map[7].escape_seq = tgetstr( "kP", &ptr ); /* PREV */
	keypad_map[7].mapped[0] = '-';
	keypad_map[7].mapped[1] = '\0';
	keypad_map[8].escape_seq = tgetstr( "ta", &ptr ); /* TAB  */
	keypad_map[8].mapped[0] = 'j';
	keypad_map[8].mapped[1] = '\0';
	keypad_map[9].escape_seq = tgetstr( "bt", &ptr ); /* BACK TAB*/
	keypad_map[9].mapped[0] = 'k';
	keypad_map[9].mapped[1] = '\0';
	keypad_map[0].escape_seq = tgetstr( "ku", &ptr ); /* UP   */
	keypad_map[0].mapped[0] = 'k';
	keypad_map[0].mapped[1] = '\0';

	if ( strlen(keypad_map[8].escape_seq) == 0 ) {
		dflt_tab_char = " ";
		*dflt_tab_char = TAB;
		keypad_map[8].escape_seq = dflt_tab_char;
	}


#ifndef ELM
	_setbold = tgetstr( "so", &ptr );
	_clearbold = tgetstr( "se", &ptr );
	_setunderline = tgetstr( "us", &ptr );
	_clearunderline = tgetstr( "ue", &ptr );
	_sethalfbright = tgetstr( "hs", &ptr );
	_clearhalfbright = tgetstr( "he", &ptr );
#endif


	_cleartoeoln = tgetstr( "ce", &ptr );
	_cleartoeos = tgetstr( "cd", &ptr );
	_lines = tgetnum( "li" );
	_columns = tgetnum( "co" );
	_magiccookie = tgetnum( "sg" );
	_transmit_on = tgetstr( "ks", &ptr );
	_transmit_off = tgetstr( "ke", &ptr );


#ifndef ELM
	_set_memlock = tgetstr( "ml", &ptr );
	_clear_memlock = tgetstr( "mu", &ptr );
#endif


	if ( !_left ) {
		_left = ptr;
		*ptr++ = '\b';
		*ptr++ = '\0';
	}

	return ( 0 );
}


char 
*return_value_of( termcap_label )

	char           *termcap_label;

{
	/*
	 *  This will return the string kept by termcap for the 
	 *  specified capability. Modified to ensure that if 
	 *  tgetstr returns a pointer to a transient address	
	 *  that we won't bomb out with a later segmentation
	 *  fault (thanks to Dave@Infopro for this one!)
 	 * 
	 *  Tweaked to remove padding sequences.
	 */


	register int    i = 0, 
			j = 0;
	char            buffer[20],
	                *myptr;


	if ( strlen(termcap_label) < 2 )
		return ( NULL );

	if ( termcap_label[0] == 's' && termcap_label[1] == 'o' )
		strcpy( escape_sequence, _setinverse );
	else if ( termcap_label[0] == 's' && termcap_label[1] == 'e' )
		strcpy( escape_sequence, _clearinverse );
	else if ( (myptr = tgetstr(termcap_label, &ptr)) == NULL )
		return ( (char *) NULL );
	else
		strcpy( escape_sequence, myptr );

	if ( chloc(escape_sequence, '$') != -1 ) {
		while ( escape_sequence[i] != '\0' ) {
			while ( escape_sequence[i] != '$' && escape_sequence[i] != '\0' )
				buffer[j++] = escape_sequence[i++];
			if ( escape_sequence[i] == '$' ) {
				while ( escape_sequence[i] != '>' )
					i++;
				i++;
			}
		}

		buffer[j] = '\0';
		strcpy( escape_sequence, buffer );
	}

	return ( (char *) escape_sequence );
}


int 
transmit_functions( newstate )

	int             newstate;

{
	/*
	 *  turn function key transmission to ON | OFF 
	 */


	if ( newstate != _intransmit ) {
		_intransmit = !_intransmit;

		if ( newstate == ON )
			tputs( _transmit_on, 1, outchar );
		else
			tputs( _transmit_off, 1, outchar );

		fflush(stdout);			/* clear the output buffer */
	}
}

/*
 *******************************************************************
        now into the 'meat' of the routines...the cursor stuff 
 *******************************************************************
 */


int 
ScreenSize( lines, columns )

	int             *lines, 
			*columns;

{
	/*
	 *  returns the number of lines and columns on the display. 
	 */


	if ( _lines == 0 )
		_lines = DEFAULT_LINES_ON_TERMINAL;

	if ( _columns == 0 )
		_columns = DEFAULT_COLUMNS_ON_TERMINAL;

	*lines = _lines - 1;	/* assume index from zero for cursor position*/
	*columns = _columns;    /* used for the number of char per line      */
}


int
GetXmcStatus()

{
	/*
	 * return magic_cookie_glitch value of the terminal.
	 * This value indicate the number of blank characters
	 * left by 'smso' or 'rmso'(_setinverse or _clearinverse).
	 * If magic-cookie is greater than zero, it needs another
	 * backspaces or left-movement to clear the enhanced line.
	 */


	if ( has_highlighting )
		return( _magiccookie < 0 ? 0 : _magiccookie );
	else
		return( 0 );
}


int 
GetXYLocation( x, y )
 
	int             *x, 
			*y;

{
	/*
	 * return the current cursor location on the screen 
	 */


	*x = _line;
	*y = _col;
}


int 
ClearScreen()

{
	/*
	 * clear the screen: returns -1 if not capable 
	 */


	_line = 0;			/* clear leaves us at top... */
	_col = 0;

	if ( !_clearscreen )
		return ( -1 );

	tputs( _clearscreen, 1, outchar );
	fflush( stdout );			/* clear the output buffer */
	return ( 0 );
}


int 
MoveCursor( row, col )

	int             row, 
			col;

{
	/*
	 *  move cursor to the specified row column on the screen.
         *  0,0 is the top left! 
	 *
	 *  NOTE: 
	 *  This routine uses diffent way to move cursor.
	 *  The criteria of which way to use is the relation
	 *  between current position and the point to move.
	 *  If the point to move is within 5 characters up or 
	 *  down or right or left, then this uses CursorUp,
	 *  CursorDown, CursorRight, CursorLeft, respectively.
	 *  The reason of this selection is the length of 
	 *  the HP terminal escape sequence, perhaps.
	 *  (e.g. ESC&a<yyy>R<xxx>C for cursor move is longer
	 *  than 4 times of ESCA for up arrow.)
	 *  Moreover the current position is indicated the values of
	 *  _line and _col which are updated only by functions in
	 *  this file. Because of this, movement of cursor without
	 *  using routine in this file may cause unexpected cursor
	 *  movement.
	 *  If you want to set cursor as you like anyway, use 
	 *  another function "SetCursor".
	 */


	char            *stuff, 
			*tgoto();


	/*
	 * we don't want to change "rows" or we'll mangle scrolling... 
	 */

	if ( col >= COLUMNS )
		col = COLUMNS - 1; 			/* COLUMNS means number of */
							/* chars per line.         */
	if ( col < 0 )
		col = 0;

	if ( !_moveto )
		return ( -1 );

	if ( row == _line ) {
		if ( col == _col )
			return ( 0 );			/* already there! */

		else if ( abs(col - _col) < 5 ) {	/* within 5 spaces... */
			if ( col > _col )
				CursorRight( col - _col );
			else
				CursorLeft( _col - col );

		} else {				/* move along to the new x,y loc */
			stuff = tgoto( _moveto, col, row );
			tputs( stuff, 1, outchar );
			fflush( stdout );
		}

	} else if ( col == _col && abs(row - _line) < 5 ) {
		if ( row < _line )
			CursorUp( _line - row );
		else
			CursorDown( row - _line );

	} else if ( _line == row - 1 && col == 0 ) {
		(void) putchar( '\n' );			
		(void) putchar( '\r' );		
		fflush( stdout );

	} else {
		stuff = tgoto( _moveto, col, row );
		tputs( stuff, 1, outchar );
		fflush( stdout );
	}

	_line = row;					/* to ensure we're really there.. */
	_col = col;

	return ( 0 );
}


int 
SetCursor( row, col )

	int             row, 
			col;

{
	/*
	 *  move cursor to the specified row column on the screen.
         *  0,0 is the top left! 
	 *
	 *  This routine set the cursor position to the place as
	 *  you wish. Or for reset the cursor position.
	 */


	char            *stuff, 
			*tgoto();


	if ( col >= COLUMNS )
		col = COLUMNS - 1;			/* COLUMNS means number of */
							/* chars per line.         */

	if ( col < 0 )
		col = 0;

	if ( !_moveto )
		return ( -1 );

	stuff = tgoto( _moveto, col, row );
	tputs( stuff, 1, outchar );
	fflush( stdout );

	_line = row;				/* to ensure we're really there... */
	_col = col;

	return ( 0 );
}


int 
CursorUp( n )
 
	int             n;

{
	/*
	 *  move the cursor up 'n' lines 
	 */


	_line = ( _line - n > 0 ? _line - n : 0 );	/* up 'n' lines... */

	if ( !_up )
		return ( -1 );

	while ( n-- > 0 )
		tputs( _up, 1, outchar );

	fflush( stdout );
	return ( 0 );
}


int 
CursorDown( n )

	int             n;

{
	/*
	 *  move the cursor down 'n' lines 
	 */

	_line = ( _line + n < LINES ? _line + n : LINES );	/* down 'n' lines... */

	if ( !_down )
		return ( -1 );

	while ( n-- > 0 )
		tputs( _down, 1, outchar );

	fflush( stdout );
	return ( 0 );
}


int 
CursorLeft( n )

	int             n;

{
	/*
	 *  move the cursor 'n' characters to the left 
	 */

	_col = ( _col - n > 0 ? _col - n : 0 );		/* left 'n' chars... */

	if ( !_left )
		return ( -1 );

	while ( n-- > 0 )
		tputs( _left, 1, outchar );

	fflush( stdout );
	return ( 0 );
}


int 
CursorRight( n )

	int             n;

{
	/*
	 *  move the cursor 'n' characters to the right (nondestructive) 
	 */

	_col = ( _col + n < COLUMNS ? _col + n : COLUMNS-1 );	/* right 'n' chars... */

	if ( !_right )
		return ( -1 );

	while ( n-- > 0 )
		tputs( _right, 1, outchar );

	fflush( stdout );
	return ( 0 );
}


int 
StartInverse()

{
	/*
	 *  start inverse mode 
	 */

	if ( !_setinverse )
		return ( -1 );

	tputs( _setinverse, 1, outchar );
	fflush( stdout ); 
	return ( 0 ); 
}


int 
EndInverse()

{
	/*
	 *  compliment of startinverse 
	 */

	if ( !_clearinverse )
		return ( -1 );

	tputs( _clearinverse, 1, outchar );
	fflush( stdout ); 
	return ( 0 );
}


#ifndef ELM

int
HasMemlock()

{
	/*
	 *  returns TRUE iff memory locking is available (a terminal
	 *  feature that allows a specified portion of the screen to
	 *  be "locked" & not cleared/scrolled... 
	 */

	return ( _set_memlock && _clear_memlock );
}


static int      _old_LINES;


int
StartMemlock()

{
	/*
	 *  mark the current line as the "last" line of the portion to 
	 *  be memory locked (always relative to the top line of the
	 *  screen) Note that this will alter LINES so that it knows
	 *  the top is locked.  This means that (plus) the program 
	 *  will scroll nicely but (minus) End memlock MUST be called
	 *  whenever we leave the locked-memory part of the program! 
	 */


	if ( !_set_memlock )
		return ( -1 );

	if ( !_memory_locked ) {

		_old_LINES = LINES;
		LINES -= _line;			/* we can't use this for scrolling */

		tputs( _set_memlock, 1, outchar );
		fflush( stdout );
		_memory_locked = TRUE;
	}

	return ( 0 );
}


int
EndMemlock()

{
	/*
	 *  Clear the locked memory condition...  
	 */


	if ( !_set_memlock )
		return ( -1 );

	if ( _memory_locked ) {
		LINES = _old_LINES;		/* back to old setting */

		tputs( _clear_memlock, 1, outchar );
		fflush( stdout );
		_memory_locked = FALSE;
	}

	return ( 0 );
}

#endif


int 
Writechar( ch )

	unsigned char        ch;

{
	/*
	 *  write a character to the current screen location. 
	 */

	(void) putchar( ch );

	if ( ch == (unsigned char)BACKSPACE )	/* moved BACK one! */
		_col--;
	else if ( ch >= (unsigned char)' ' )	/* moved FORWARD one! */
		_col++;
}

/* VARARGS2 */

int 
Write_to_screen( line, argcount, arg1, arg2, arg3 )

	char            *line;
	int             argcount;
	char            *arg1, 
			*arg2, 
			*arg3;
{
	/*
	 *  This routine writes to the screen at the current location.
  	 *  when done, it increments lines & columns accordingly by
	 *  looking for "\n" sequences... 
	 */


	char	 	buffer[VERY_LONG_STRING],
			buffer1[VERY_LONG_STRING];
	size_t		len;


	if ( has_highlighting ){
		len = strlen( _setinverse );

		if ( _magiccookie > 0 && strncmp(arg1, _setinverse, len) == 0 ){

		/*
		 * where the enhanced line has 2 leading blank char. 
		 * If the terminal has magic-cookie in terminfo,
		 * it place blank char with enhance char. On header 
		 * page, it will not start at the same colomn because
		 * of this. To avoid this ...
		 */

			strncpy( buffer, arg1, len );
			buffer[len] = '\0';

			if ( strlen(arg1) < VERY_LONG_STRING )
				strcat( buffer, arg1
					+(unsigned long)(_magiccookie >2 ? 2:_magiccookie)
					+(unsigned long)len );
			else {
				strncpy( buffer1, arg1, VERY_LONG_STRING-1 );
				buffer1[VERY_LONG_STRING-1] = '\0';
				strcat( buffer, buffer1
					+(unsigned long)(_magiccookie >2 ? 2:_magiccookie)
					+(unsigned long)len );
			}

			strcpy( arg1, buffer );
		} 
	}

	switch ( argcount ) {
	case 0:
		PutLine0( _line, _col, line );
		break;

	case 1:
		PutLine1( _line, _col, line, arg1 );
		break;

	case 2:
		PutLine2( _line, _col, line, arg1, arg2 );
		break;

	case 3:
		PutLine3( _line, _col, line, arg1, arg2, arg3 );
		break;
	}
}


int 
PutLine0( x, y, line )
	
	int             x, 
			y;
	char           *line;

{
	/*
	 *  Write a zero argument line at location x,y 
	 */

	char            *p, 
			*pend;


	MoveCursor( x, y );
	printf( "%s", line );			/* to avoid '%' problems */
	fflush( stdout );

	_col += printable_chars( (unsigned char *)line );

	if ( has_highlighting && !arrow_cursor && first_word(line, start_highlight) )
		_col = _col - printable_chars((unsigned char *)start_highlight)
			    - printable_chars((unsigned char *)end_highlight);

	while ( _col >= COLUMNS ) {		/* line wrapped around?? */
		_col -= COLUMNS;
		_line += 1;

		if ( _line > LINES )
			_line = LINES;		/* max line */
	}

	/*
	 *  now let's figure out if we're supposed to do a "<return>" 
	 */

	pend = line + strlen( line );

	for ( p = line; p < pend; )

		if ( *p++ == '\n' ) {
			_line++;

			if ( _line > LINES )
				_line = LINES;	/* max line */

			_col = 0;		/* on new line! */
		}
}


/* VARARGS2 */

int
PutLine1( x, y, line, arg1 )

	int             x, 
			y;
	char           *line,
	               *arg1;

{
	/*
	 *  write line at location x,y - one argument... 
	 */


	char            buffer[VERY_LONG_STRING];


	sprintf( buffer, line, arg1 );

	PutLine0( x, y, buffer );
}


/* VARARGS2 */

int
PutLine2( x, y, line, arg1, arg2 )

	int             x, 
			y;
	char            *line,
	                *arg1, 
			*arg2;

{
	/*
	 *  write line at location x,y - one argument... 
	 */


	char            buffer[VERY_LONG_STRING];


	sprintf( buffer, line, arg1, arg2 );

	PutLine0( x, y, buffer );
}


/* VARARGS2 */

int
PutLine3( x, y, line, arg1, arg2, arg3 )

	int             x, 
			y;
	char            *line,
	                *arg1, 
			*arg2, 
			*arg3;

{
	/*
	 *  write line at location x,y - one argument... 
	 */


	char            buffer[VERY_LONG_STRING];


	sprintf( buffer, line, arg1, arg2, arg3 );

	PutLine0( x, y, buffer );
}


int 
CleartoEOLN()

{
	/*
	 *  clear to end of line 
	 */


	if ( !_cleartoeoln )
		return ( -1 );

	tputs( _cleartoeoln, 1, outchar );
	fflush( stdout );			/* clear the output buffer */
	return ( 0 );
}


int 
CleartoEOS()

{
	/*
	 *  clear to end of screen 
	 */


	if ( !_cleartoeos )
		return ( -1 );

	tputs( _cleartoeos, 1, outchar );
	fflush( stdout );			/* clear the output buffer */
	return ( 0 );
}


#ifdef RAWMODE

/*
 * #ifndef ELM	 elm doesn't use this function 
 */

int 
RawState()

{
	/*
	 *  returns either 1 or 0, for ON or OFF 
	 */

	return ( _inraw );
}


/* #endif */

int 
Raw( state )

	int             state;

{
	/*
	 *  state is either ON or OFF, as indicated by call 
	 */


	if ( state == OFF && _inraw ) {
		(void) ioctl( TTYIN, TCSETAW, &_original_tty );
		_inraw = 0;
	} else if ( state == ON && !_inraw ) {

		(void) ioctl( TTYIN, TCGETA, &_original_tty );	/** current setting **/

		(void) ioctl( TTYIN, TCGETA, &_raw_tty );	/** again! **/

		_raw_tty.c_lflag &= ~(ICANON | ECHO);		/* noecho raw mode        */
		_raw_tty.c_oflag &= ~(ONLCR);			/* stop mapping NL->CRNL  */

		_raw_tty.c_cc[VMIN] = '\01';			/* minimum # of chars to
						 	 	 * queue    */
		_raw_tty.c_cc[VTIME] = '\0';			/* minimum time to wait for
								 * input */

		(void) ioctl( TTYIN, TCSETAW, &_raw_tty );

		_inraw = 1;
	}
      }


jmp_buf		fastkey;


int
ReadCh()

{
	/*
	 *  read a character with Raw mode set ! 
	 * 
	 *
	 *  This routine was enhanced for fast key input.
	 *  If user input "2 bytes character(like NLS char)",
	 *  elm read only the first character. This causes
	 *  an action  for character of first byte, moreover
	 *  the second character is understood as a different 
	 *  command, too. To avoid this kind of confuse, add 
	 *  timeout functionality. The character sequence
	 *  which has larger interval than the timer is assumed
	 *  inputed separately, not by a keypad.
	 */


	register int	i,j,
			suffix,
			esc_flag = 0;
	char		cmd_ret,
			cmd_tmp;
	void		(*old_alarm)();


	if ( read(0, &cmd_ret, 1) != 1 )
	        cmd_ret = NO_OP_COMMAND;
	else {

		for ( i = 0; i < 10; i++ )
			if ( cmd_ret == *keypad_map[i].escape_seq ){
				suffix = i;
				esc_flag++;
			}

		if ( esc_flag <= 1 ) 
			return( cmd_ret );

		old_alarm = signal( SIGALRM, trapalarm );

		j = 1;
		while (TRUE) {
			(void)alarm( (unsigned long)0 ); /* make sure */

			setalarm();

			if ( setjmp(fastkey) == 0 ){

				if ( read(0,&cmd_tmp,1) != 1 ) {
					(void) alarm( (unsigned long)0 );
					cmd_ret = NO_OP_COMMAND;
					break;
				}

				(void) alarm( (unsigned long)0 );

				esc_flag = 0;

				for ( i = 0; i < 10; i++ )
					if (cmd_tmp==*(keypad_map[i].escape_seq+j)){
						suffix = i;
						esc_flag++;
					}

				if ( esc_flag == 1 ) {
					cmd_ret = keypad_map[suffix].mapped[0];
					break;
				}

				if ( esc_flag == 0 ) {
					cmd_ret = NO_OP_COMMAND;
					break;
				}

			} else
				break;

			j++;
		}

		if ( !cursor_control || !hp_terminal )
			cmd_ret = NO_OP_COMMAND;

		signal( SIGALRM, old_alarm );
	}

	return( cmd_ret );
}



int 
setalarm()

{

	struct itimerval	itimer;

	itimer.it_interval.tv_sec = 0;
	itimer.it_interval.tv_usec = 0;
	itimer.it_value.tv_sec = 0;
	itimer.it_value.tv_usec = 300000; 	/* 300 ms */

	if ( setitimer(ITIMER_REAL, &itimer, (struct itimerval *)0) )
		(void) alarm( (unsigned long)1 );
}


void
trapalarm()
{

	(void) alarm( (unsigned long)0 );
	ioctl( TTYIN, TCFLSH, 2 );
	longjmp( fastkey, 1 );
}

int
OnlcrSwitch( flag )

	int	flag;

{
	/*
	 *  switch ONLCR flag of stdin 
	 */


	struct termio	ttyin;

	if ( flag == ON ){
		ioctl( TTYIN, TCGETA, &ttyin );
		ttyin.c_oflag |= ONLCR;
		ioctl( TTYIN, TCSETAW, &ttyin );
	} else {
		ioctl( TTYIN, TCGETA, &ttyin );
		ttyin.c_oflag &= ~(ONLCR);
		ioctl( TTYIN, TCSETAW, &ttyin );
	}
}


#endif

int 
outchar( c )

	char            c;

{
	/*
	 *  output the given character.  From tputs... 
	 *  Note: this CANNOT be a macro!             
	 */


	return( putc( c, stdout ) );
}
