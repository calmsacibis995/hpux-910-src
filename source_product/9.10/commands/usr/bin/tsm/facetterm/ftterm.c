/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: ftterm.c,v 70.1 92/03/09 15:43:44 ssa Exp $ */
/**************************************************************************
* ftterm.c
*	Terminal i/o routines.
**************************************************************************/

#include <stdio.h>

#include "person.h"

#include	"ftchar.h"
#include	"myattrs.h"

extern char	*my_tparm();

                        /* needed for error check */
#include        <errno.h>
extern int      errno;

#include	"ftterm.h"
int	Rows = 0;
int	Rows_default = 0;
int	Rows_terminal = 0;
int	Rows_max = 0;
int	Row_bottom_default = 0;
int	Row_bottom_terminal = 0;
int	Row_bottom_max = 0;
int	Cols = 0;
int	Col_right = 0;
int	Cols_wide = 0;
int	Col_right_wide = 0;
int	F_has_status_line = 0;

#include	"cursor.h"
int	Cursor_address_wide_starts[ MAX_PERSONALITY ] = { 0 };

#include	"features.h"
#include	"extra.h"
#include	"modes.h"
#include	"ftwindow.h"

char	*S_linefeed = "\n";
char	*S_bell = "\007";
char	*S_backspace= "\b";

#include "terminal.h"
char	*Term;
char	*Facetterm;
#define MAX_FT_LEN 11
char	Facetterm_shortened[ MAX_FT_LEN + 1 ];

#include "options.h"

int	F_tty_is_CS8 = 0;

char	*Text_term_not_set = 
"Cannot determine terminal type. Environment variable TERM must be set.\r\n";

char	*Text_term_type_is =
"Terminal type is: %s.     FacetTerm Terminal type is: %s.\r\n\n";

char	*Text_reading_terminal_desc = 
"Reading terminal description file...\n";

char	*Text_hotkey_disabled =
"FacetTerm 'window command hotkey' is disabled -  Press Return to start: ";

char	*Text_hotkey_is_control =
"FacetTerm 'window command hotkey' is Control-%c  -  Press Return to start: ";

char	*Text_hotkey_is_char =
"FacetTerm 'window command hotkey' is '%c'  -  Press Return to start: ";

char	*Text_menu_hotkey_is_control =
"FacetTerm 'menu hotkey' is Control-%c\n";

char	*Text_menu_hotkey_is_char =
"FacetTerm 'menu hotkey' is '%c'\n";
/**************************************************************************
* term_init
*       Setup terminal line, read terminal description file,
*       get user acknowlegement, finish setup of terminal.
**************************************************************************/
term_init()
{
	int	status;
	char	buff[ 80 ];
	char	*getenv();
	int	rows_change_max;
	int	personality;

	printf( "\n" );
	fct_decode_init();
	fct_key_init();
	fct_attribute_init();
	Term=getenv( "TERM" );
	if ( Term == NULL )
	{
/*"Cannot determine terminal type. Environment variable TERM must be set.\r\n"*/
		printf( Text_term_not_set );
		wait_return_pressed();
		return( -1 );
	}
	Facetterm=getenv( "FACETTERM" );
	if ( Facetterm == NULL )
	{
		char	*tsmterm;

		tsmterm = getenv( "TSMTERM" );
		if ( tsmterm != NULL )
			Facetterm = tsmterm;
	}
	if ( Facetterm == NULL )
		Facetterm = Term;
	if ( strlen( Facetterm ) > MAX_FT_LEN )
	{
		strncpy( Facetterm_shortened, Facetterm, MAX_FT_LEN );
		Facetterm_shortened[ MAX_FT_LEN ] = '\0';
		Facetterm = Facetterm_shortened;
	}
/* "Terminal type is: %s.     FacetTerm Terminal type is: %s.\r\n\n" */
	if ( Opt_nonstop == 0 )
		printf( Text_term_type_is, Term, Facetterm );

/* "Reading terminal description file...\n" */
	if ( Opt_nonstop )
		printf( Text_reading_terminal_desc );

	status = get_extra( Facetterm );
	if ( status != 0 )
	{
		wait_return_pressed();
		return( -1 );
	}
	status = term_check_load();
	get_environment_pages();
	get_environment_rows();
	get_environment_cols();
	if ( ( Rows <= 0 ) || ( Rows > MAX_ROWS ) )
	{
		printf( "Value for lines is invalid: %d. Max is: %d\n",
			Rows, MAX_ROWS );
		wait_return_pressed();
		return( -1 );
	}
	Rows_default =	Rows;
	Rows_terminal =	Rows;
	Rows_max =	Rows;
	rows_change_max = get_rows_change_max();
	if ( rows_change_max > Rows_max )
		Rows_max = rows_change_max;
	if ( F_has_status_line )
		Rows_max = Rows + 1;
	if ( ( Rows_max <= 0 ) || ( Rows_max > MAX_ROWS ) )
	{
		printf( "Value for rows is invalid: %d. Max is: %d\n",
			Rows_max, MAX_ROWS );
		wait_return_pressed();
		return( -1 );
	}
	Row_bottom_default =	Rows - 1;
	Row_bottom_terminal =	Rows_terminal - 1;
	Row_bottom_max =	Rows_max - 1;
	term_init_memory_lock();
	term_init_change_scroll_region( 0, Row_bottom_terminal );
	if ( ( Cols <= 0 )  || ( Cols > MAX_COLS ) )
	{
		printf( "Value for columns is invalid: %d. Max is: %d.\n",
			Cols, MAX_COLS );
		wait_return_pressed();
		return( -1 );
	}
	Col_right = Cols - 1;
	for ( personality = 0; personality <= X_pe; personality++ )
	{
		if ( Cursor_address_wide_starts[ personality ] <= 0 )
			Cursor_address_wide_starts[ personality ] = Cols;
	}
	if ( Cols_wide <= 0 )
		Cols_wide = Cols;
	if ( Cols_wide > MAX_COLS)
	{
		printf( "Value for columns_wide is invalid: %d. Max is: %d.\n",
			Cols_wide, MAX_COLS );
		wait_return_pressed();
		return( -1 );
	}
	Col_right_wide = Cols_wide - 1;
	term_init_set_margins( 0, Col_right );
	term_init_terminal_mode();
	if ( status != 0 )
	{
		printf( "Not enough information to run\r\n" );
		wait_return_pressed();
		return( -1 );
	}
	printf( "\r\n" );
	F_tty_is_CS8 = 
		termio_init( F_xon_xoff, F_force_CS8, F_no_clear_ISTRIP,
				Opt_use_PARMRK_for_break,
				F_allow_tabs );
	if ( F_tty_is_CS8 < 0 )
	{
		wait_return_pressed();
		return( -1 );
	}
	term_check_terminal_mode( F_tty_is_CS8 );
	if ( Opt_nonstop == 0 )
	{
		if ( Opt_menu_hotkey_set && ( Opt_menu_hotkey > 0 ) )
		{
			if ( Opt_menu_hotkey < 0x40 )
			{
/* "FacetTerm 'menu hotkey' is Control-%c\n" */
			printf( Text_menu_hotkey_is_control,
					(char) (Opt_menu_hotkey + 0x40 ));
			}
			else
			{
/* "FacetTerm 'menu hotkey' is '%c'\n" */
				printf( Text_menu_hotkey_is_char, 
					(char) Opt_menu_hotkey );
			}
		}
		/* NO_PROMPT ??? */
		if ( Opt_hotkey < 0 )
		{
/* "FacetTerm 'command line hotkey' is disabled -  Press Return to start: " */
			printf( Text_hotkey_disabled );
		}
		else if ( Opt_hotkey < 0x40 )
		{
/* "FacetTerm 'command line hotkey' is Control-%c  -  Press Return to start: "*/
			printf( Text_hotkey_is_control,
				(char) (Opt_hotkey + 0x40 ));
		}
		else
		{
/* "FacetTerm 'command line hotkey' is '%c'  -  Press Return to start: " */
			printf( Text_hotkey_is_char, (char) Opt_hotkey );
		}
		term_outgo();
		gets( buff );
	}
	send_init_strings();
	termio_window();
	term_enter_ca_mode();
	term_outgo();
	return( 0 );
}
/**************************************************************************
* get_environment_rows
*	User can override the number of lines on the terminal screen from
*	the environment. Check for this case and set appropriate variables.
**************************************************************************/
get_environment_rows()
{
	char	*p;
	int	rows;
	char	*getenv();

	p = getenv( "FACETLINES" );
	if ( p != NULL )
	{
		rows = atoi( p );
		if ( rows > 0 )
			Rows = rows;
		return;
	}
	p = getenv( "TSMLINES" );
	if ( p != NULL )
	{
		rows = atoi( p );
		if ( rows > 0 )
			Rows = rows;
	}
}
/**************************************************************************
* get_environment_cols
*	User can override the number of columns on the terminal screen from
*	the environment. Check for this case and set appropriate variables.
**************************************************************************/
get_environment_cols()
{
	char	*p;
	int	cols;
	char	*getenv();

	p = getenv( "FACETCOLUMNS" );
	if ( p != NULL )
	{
		cols = atoi( p );
		if ( cols > 0 )
			Cols = cols;
		return;
	}
	p = getenv( "TSMCOLUMNS" );
	if ( p != NULL )
	{
		cols = atoi( p );
		if ( cols > 0 )
			Cols = cols;
	}
}
char	*Text_press_return_to_exit =
	"Press RETURN to exit.\r\n";
/**************************************************************************
* wait_return_pressed
*	Allow user to read fatal error message.
**************************************************************************/
wait_return_pressed()
{
	char	buff[ 80 ];

		/* "Press RETURN to exit.\r\n" */
	printf( Text_press_return_to_exit );
	fflush( (FILE *) stdout );
	gets( buff );
}
char	*Text_press_return_to_continue =
	"Press RETURN to continue: ";		/* TBD */
/**************************************************************************
* wait_return_pressed_continue
*	Allow user to read important warning message.
**************************************************************************/
wait_return_pressed_continue()
{
	char	buff[ 80 ];

		/* "Press RETURN to continue: " */
	printf( Text_press_return_to_continue );
	fflush( (FILE *) stdout );
	gets( buff );
}

char	*Text_split_screen_disabled =
	"Notice: Split screen is disabled.\n";
/**************************************************************************
* term_check_load
*	Determine that all necessary information from the terminal description
*	file and environment is present and consistent.
**************************************************************************/
term_check_load()
{
	int	status;
	int	result;
	int	personality;

	result = 0;

	for ( personality = 0; personality <= X_pe; personality++ )
	{
		status = cap_need( T_cursor_address[ personality ], 
							"cursor_address" );
		if ( status )
			result = status;

		if ( pT_cursor_down == NULL )
			pT_cursor_down = "\n";

		if ( pT_cursor_left == NULL )
			pT_cursor_left = "\b";

		if (  ( pT_out_clr_eol == NULL )
		   && ( pT_clr_eol == NULL ) 
		   && ( pT_clr_eol_w_attr == NULL )
		   && ( pT_clr_eol_unprotected == NULL )
		   && ( pT_clr_eol_unprotected_w_attr == NULL )
		   && ( pT_clr_fld_unprotected == NULL )
		   && ( pT_clr_fld_unprotected_w_attr == NULL ) )
		{
			printf( 
"Must have out_clr_eol, clr_eol [_w_attr], clr_eol_unprotected [_w_attr]\n");
			printf( "       or clr_fld_unprotected [_w_attr]\n");
			result = -1;
		}

		if (  ( pT_out_clr_eos == NULL )
		   && ( pT_clr_eos == NULL ) 
		   && ( pT_clr_eos_w_attr == NULL )
		   && ( pT_clr_eos_unprotected == NULL )
		   && ( pT_clr_eos_unprotected_w_attr == NULL ) )
		{
			printf( 
"Must have out_clr_eos, clr_eos [_w_attr], or clr_eos_unprotected [_w_attr].\n");
			result = -1;
		}

		if (  ( pT_out_clear_screen == NULL ) 
		   && ( pT_clear_all == NULL )
		   && ( pT_clear_all_w_attr == NULL )
		   && ( pT_clr_eos == NULL ) )
		{
			printf( 
	"Must have clear_screen, clear_all, clear_all_w_attr or clr_eos.\n" );
			result = -1;
		}

		if (  ( pT_change_scroll_region != NULL )
		   && ( pT_scroll_forward != NULL )
		   && ( pT_scroll_reverse != NULL ) )
		{
			pF_use_csr = 1;
		}
		else if (  ( pT_insert_line != NULL )
			&& ( pT_delete_line != NULL ) )
		{
			pF_use_csr = 0;
		}
		else
		{
			pF_use_csr = 0;
			F_no_split_screen = 1;
		}
	}
	return( result );
}
/**************************************************************************
* cap_need
*	Check for the presence of a pointer that is a required terminal
*	capability.  Complain and return -1 if it is not.
*	Otherwise, return 0 for ok.
**************************************************************************/
cap_need( ptr, name )
	char	*ptr;
	char	*name;
{
	if ( ptr == NULL )
	{
		printf( "%-20.20s not found - must be defined\n", name );
		return( -1 );
	}
	return( 0 );
}
#include "facetpath.h"
/**************************************************************************
* send_init_strings
*	Run the progream /usr/facetterm/sys/fct_init to allow system dependent
*	initialization of the terminal.
**************************************************************************/
send_init_strings()
{
	char	command[ 512 ];

	sprintf( command,"%s/sys/%sinit", Facettermpath, Facetprefix );
	mysystem( command );
}
/* ========================= TERMINAL CONTROL ROUTINES =============== */
/**************************************************************************
* term_putc
*	Output the character "c" to the terminal.
*	The stdio package is used to buffer characters which are flushed
*	when the program is going to wait or when direct i/o with write
*	is going to be used.
**************************************************************************/
term_putc( c )
	char	c;
{
	putchar( c );
}
/**************************************************************************
* term_puts
*	Output the null terminated string "s" to the terminal, after 
*	flushing any possible buffered characters.
**************************************************************************/
term_puts( s )
	char	*s;
{
	term_outgo();
	write( 1, s, (unsigned ) strlen( s ) );
}
/**************************************************************************
* term_write
*	Output the string "s" of length "len" to the terminal, after 
*	flushing any possible buffered characters.
**************************************************************************/
term_write( s, len )
	char	*s;
	int	len;
{
	term_outgo();
	write( 1, s, len );
}
/**************************************************************************
* term_outgo
*	Flush any buffered characters to the terminal.
**************************************************************************/
term_outgo()
{
	fflush( (FILE *) stdout );
}
/**************************************************************************
* term_tputs
*	Output the string "s" doing the equivalent processing of the
*	terminfo routine "tput" assuming one line is affected.
**************************************************************************/
term_tputs( s )
	char *s;
{
	my_tputs( s, 1 );
}
/**************************************************************************
* term_pos_puts
*	Output the string "s" at position "row" and "col" on the screen.
**************************************************************************/
term_pos_puts( row, col, s )
	int	row;
	int	col;
	char *s;
{
	term_pos( row, col );
	term_puts( s );
}
/**************************************************************************
* term_pos_clrlin_puts
*	Clear the row "row" on the screen and output the string "s" starting
*	in column 0;
**************************************************************************/
term_pos_clrlin_puts( row, s )
	int	row;
	char *s;
{
	term_pos( row, 0 );
	out_term_clr_eol();
	term_line_attribute_off();
	term_puts( s );
}
/**************************************************************************
* out_term_clr_row_bottom_terminal
*	Clear bottom line of terminal and any memory below it.
**************************************************************************/
out_term_clr_row_bottom_terminal()
{
	term_pos( Row_bottom_terminal, 0 );
	if ( out_term_clr_memory_below() < 0 )
		out_term_clr_eol();
	term_line_attribute_off();
}
/**************************************************************************
* make_out_cursor_address
*	Construct a string that when fed through the decoder will be 
*	understood as positioning the cursor to "row" and "col" on the
*	screen.  Return the result in the string "string" which can hold
*	a maximum of "max" characters.
**************************************************************************/
make_out_cursor_address( string, max, row, col )
	char	*string;
	int	max;
	int	row;
	int	col;
{
	char	*p;
	char	*cursorstr;
	int	parm[ 2 ];
	char	*string_parm[ 1 ];		/* not used */

	cursorstr = T_cursor_address[ O_pe ];
	if (  ( col >= Cursor_address_wide_starts[ O_pe ] ) 
	   && ( T_cursor_address_wide[ O_pe ] != NULL ) )
	{
		cursorstr = T_cursor_address_wide[ O_pe ];
	}
	parm[ 0 ] = row;
	parm[ 1 ] = col;
	p = my_tparm( cursorstr, parm, string_parm, Outwin->number );
	nopad_strncpy( string, p, max );
	return( 0 );
}
/**************************************************************************
* term_pos
*	Position the cursor to "row" and "col" on the screen.
*	If the cursor is to the right of the capability of the
*	narrow cursor addressing, use the wide cursor addressing.
*	Note that this is not present on all descriptions.
**************************************************************************/
term_pos( row, col )
	int	row;
	int	col;
{
	char	*p;
	char	*cursorstr;
	int	parm[ 2 ];
	char	*string_parm[ 1 ];		/* not used */

	cursorstr = T_cursor_address[ M_pe ];
	if (  ( col >= Cursor_address_wide_starts[ M_pe ] ) 
	   && ( T_cursor_address_wide[ M_pe ] != NULL ) )
	{
		cursorstr = T_cursor_address_wide[ M_pe ];
	}
	parm[ 0 ] = row;
	parm[ 1 ] = col;
	p = my_tparm( cursorstr, parm, string_parm, -1 );
	my_tputs( p, 1 );
}
/**************************************************************************
* term_column_address
*	Position to column "col" on the screen.
*	Note that this is not present on all terminal descriptions.
**************************************************************************/
term_column_address( col )
	int	col;
{
	char	*p;
	int	parm[ 2 ];			/* 1 used - must be >= 2 - %i */
	char	*string_parm[ 1 ];		/* not used */

	if ( mT_column_address == NULL )
		return( -1 );			/* don't know how */
	parm[ 0 ] = col;
	p = my_tparm( mT_column_address, parm, string_parm, -1 );
	my_tputs( p, 1 );
	return( 0 );				/* ok */
}
/**************************************************************************
* term_linefeed
*	Output a linefeed to the terminal.
**************************************************************************/
term_linefeed()
{
	term_tputs( S_linefeed );
}
/**************************************************************************
* term_carriage_return
*	Output a carriage return to the terminal.
**************************************************************************/
term_carriage_return()
{
	term_tputs( T_carriage_return );
}
/**************************************************************************
* term_backspace
*	Output a backspace to the terminal.
**************************************************************************/
term_backspace()
{
	term_tputs( S_backspace );
}
/**************************************************************************
* term_beep
*	Output a bell to the terminal
**************************************************************************/
term_beep()
{
	term_tputs( S_bell );
}
/**************************************************************************
* term_out_parm_insert_line
*	Output an internally generated multiple insert line to the
*	terminal.  Use the special string for internally generated if it
*	is present.
**************************************************************************/
term_out_parm_insert_line( rows, affrow )
	int	rows;
	int	affrow;			/* top line affected for delays */
{
	int	i;
	int	affcnt;		
	char	*p;
	int	parm[ 2 ];			/* 1 used - must be >= 2 - %i */
	char	*string_parm[ 1 ];		/* not used */

	affcnt = Rows_terminal - affrow;
	if ( (rows > 1) && (mT_out_parm_insert_line != NULL) )
	{
		parm[ 0 ] = rows;
		p = my_tparm( mT_out_parm_insert_line, parm, string_parm, -1 );
		my_tputs( p, affcnt );
		return;
	}
	for ( i = 0; i < rows; i++ )
		term_out_insert_line( affrow );
}
/**************************************************************************
* term_out_insert_line
*	Output an internally generated insert line to the terminal.
*	Use the special string for internally generated if it is present.
**************************************************************************/
term_out_insert_line( affrow )
	int	affrow;			/* top row affected for delays */
{
	int	affcnt;	

	if ( mT_out_insert_line != NULL )
	{
		affcnt = Rows_terminal - affrow;
		my_tputs( mT_out_insert_line, affcnt );
	}
	else
		term_insert_line( affrow );
}
/**************************************************************************
* term_insert_line
*	Output a sequence to insert a line on the terminal.
**************************************************************************/
term_insert_line( affrow )
	int	affrow;			/* top row affected for delays */
{
	int	affcnt;	

	affcnt = Rows_terminal - affrow;
	if ( mT_insert_line != NULL )
		my_tputs( mT_insert_line, affcnt );
	else
		my_tputs( "INSERT_LINE", affcnt );
}
/**************************************************************************
* term_parm_insert_line
*	Insert "rows" number of lines on the terminal.
**************************************************************************/
term_parm_insert_line( rows, affrow )
	int	rows;
	int	affrow;			/* top line affected for delays */
{
	int	i;
	int	affcnt;		
	char	*p;
	int	parm[ 2 ];			/* 1 used - must be >= 2 - %i */
	char	*string_parm[ 1 ];		/* not used */

	affcnt = Rows_terminal - affrow;
	if ( (rows > 1) && (mT_parm_insert_line != NULL) )
	{
		parm[ 0 ] = rows;
		p = my_tparm( mT_parm_insert_line, parm, string_parm, -1 );
		my_tputs( p, affcnt );
		return;
	}
	for ( i = 0; i < rows; i++ )
		term_insert_line( affrow );
}
/**************************************************************************
* term_out_parm_delete_line
*	Output an internally generated multiple delete line to the
*	terminal.  Use the special string for internally generated if it
*	is present.
**************************************************************************/
term_out_parm_delete_line( rows, affrow )
	int	rows;
	int	affrow;			/* top line affected for delays */
{
	int	i;
	int	affcnt;	
	char	*p;
	int	parm[ 2 ] ;			/* 1 used - must be >= 2 - %i */
	char	*string_parm[ 1 ];		/* not used */

	affcnt = Rows_terminal - affrow;		
	if ( (rows > 1) && (mT_out_parm_delete_line != NULL) )
	{
		parm[ 0 ] = rows;
		p = my_tparm( mT_out_parm_delete_line, parm, string_parm, -1 );
		my_tputs( p, affcnt );
		return;
	}
	for ( i = 0; i < rows; i++ )
		term_out_delete_line( affrow );
}
/**************************************************************************
* term_out_delete_line
*	Output an internally generated delete line to the terminal.
**************************************************************************/
term_out_delete_line( affrow )
	int	affrow;			/* top line affected for delays */
{
	int	affcnt;	

	if ( mT_out_delete_line != NULL )
	{
		affcnt = Rows_terminal - affrow;
		my_tputs( mT_out_delete_line, affcnt );
	}
	else
		term_delete_line( affrow );
}
/**************************************************************************
* term_delete_line
*	Output a delete line to the terminal.
**************************************************************************/
term_delete_line( affrow )
	int	affrow;			/* top line affected for delays */
{
	int	affcnt;			

	affcnt = Rows_terminal - affrow;			
	if ( mT_delete_line != NULL )
		my_tputs( mT_delete_line, affcnt );
	else
		my_tputs( "DELETE_LINE", affcnt );
}
/**************************************************************************
* term_parm_delete_line
*	Output a sequence to delete "rows" number of lines on the terminal.
**************************************************************************/
term_parm_delete_line( rows, affrow )
	int	rows;
	int	affrow;			/* top line affected for delays */
{
	int	i;
	int	affcnt;	
	char	*p;
	int	parm[ 2 ] ;			/* 1 used - must be >= 2 - %i */
	char	*string_parm[ 1 ];		/* not used */

	affcnt = Rows_terminal - affrow;		
	if ( (rows > 1) && (mT_parm_delete_line != NULL) )
	{
		parm[ 0 ] = rows;
		p = my_tparm( mT_parm_delete_line, parm, string_parm, -1 );
		my_tputs( p, affcnt );
		return;
	}
	for ( i = 0; i < rows; i++ )
		term_delete_line( affrow );
}
/**************************************************************************
* term_parm_ich
*	Output a sequence to insert "cols" number of columns on the terminal.
**************************************************************************/
term_parm_ich( cols )
	int	cols;
{
	int	i;
	int	affcnt;			/* needs to be passed in ??? */
	char	*p;
	int	parm[ 2 ];			/* 1 used - must be >= 2 - %i */
	char	*string_parm[ 1 ];		/* not used */

	affcnt = 80;			/* ??? */
	if (  ( mT_parm_ich != NULL )
	   && ( ( cols > 1 ) || ( mT_insert_character == NULL ) ) )
	{
		parm[ 0 ] = cols;
		p = my_tparm( mT_parm_ich, parm, string_parm, -1 );
		my_tputs( p, affcnt );
		term_insert_padding();
	}
	else
	{
		for ( i = 0; i < cols; i++ )
			term_insert_character();
	}
}
/**************************************************************************
* term_insert_character
*	Output a sequence to insert a character on the terminal.
**************************************************************************/
term_insert_character()
{
	int	affcnt;			/* ??? needs to be passed in */

	affcnt = 1;
	if ( mT_insert_character != NULL )
		my_tputs( mT_insert_character, affcnt );
	term_insert_padding();
}
/**************************************************************************
* term_insert_padding
*	Output any padding to be done after an insert character sequence that
*	was specified in the terminal description file.
**************************************************************************/
term_insert_padding()
{
	int	affcnt;			/* ??? needs to be passed in */

	affcnt = 1;
	if ( mT_insert_padding != NULL )
		my_tputs( mT_insert_padding, affcnt );
}
/**************************************************************************
* term_enter_insert_mode
*	Output a sequence to put the terminal in insert mode.
**************************************************************************/
term_enter_insert_mode()
{

	if ( mT_enter_insert_mode != NULL  )
		term_tputs( mT_enter_insert_mode );
}
/**************************************************************************
* term_exit_insert_mode
*	Output a sequence to cause the terminal to exit insert mode.
**************************************************************************/
term_exit_insert_mode()
{
	if ( mT_exit_insert_mode != NULL )
		term_tputs( mT_exit_insert_mode );
}
/**************************************************************************
* term_se_insert_mode
*	An event has taken place which has the side effect of setting the
*	terminal's insert_mode to "insert_mode". 0=off Otherwise=on.
**************************************************************************/
/*ARGSUSED*/
term_se_insert_mode( insert_mode )
	int	insert_mode;
{
	/* not remembered for terminal */
}
/**************************************************************************
* term_delete_character
*	Output a sequence to cause the character under the cursor on the
*	terminal to be deleted.
**************************************************************************/
term_delete_character()
{
	int	affcnt;			/* needs to be passed in ??? */

	affcnt = 80;
	if ( mT_delete_character != NULL  )
		my_tputs( mT_delete_character, affcnt );
}
/**************************************************************************
* term_parm_delete_character
*	Output a sequence to cause the character under the cursor and
*	"chars" - 1 characters to the right of the cursor to be deleted.
**************************************************************************/
term_parm_delete_character( chars )
	int	chars;
{
	int	i;
	int	affcnt;			/* needs to be passed in ??? */
	char	*p;
	int	parm[ 2 ];			/* 1 used - must be >= 2 - %i */
	char	*string_parm[ 1 ];		/* not used */

	affcnt = 80;			/* ??? */
	if ( (chars > 1) && (mT_parm_delete_character != NULL) )
	{
		parm[ 0 ] = chars;
		p = my_tparm( mT_parm_delete_character, parm, string_parm, -1 );
		my_tputs( p, affcnt );
		return;
	}
	for ( i = 0; i < chars; i++ )
		term_delete_character();
}
/**************************************************************************
* term_scroll_forward
*	Output a sequence to the terminal to cause the screen to scroll.
**************************************************************************/
term_scroll_forward()
{
	my_tputs( mT_scroll_forward, 1 );
}
/**************************************************************************
* term_scroll_reverse
*	Output a sequence to cause the terminal to scroll backwards.
*	I.E. line 0 moves to line 1 etc leaving the top line blank.
**************************************************************************/
term_scroll_reverse()
{
	if ( mT_scroll_reverse != NULL )
		my_tputs( mT_scroll_reverse, 1 );
	else
		my_tputs( "SCROLLREVERSE", 1 );
}
/**************************************************************************
* make_out_cursor_home
*	Construct a string that when fed to the decoder will cause the
*	window to do a cursor home operation.  Return the result in the
*	string "string" which hold a maximum of "max" characters.
*	If successful, return  0
*	Return -1 otherwise
**************************************************************************/
make_out_cursor_home( string, max )
	char	*string;
	int	max;
{
	if ( oT_cursor_home != NULL )
	{
		nopad_strncpy( string, oT_cursor_home, max );
		return( 0 );
	}
	else
		return( make_out_cursor_address( string, max, 0, 0 ) );
}
/**************************************************************************
* term_cursor_home
*	Output a sequence to cause the cursor on the terminal to move to
*	the upper left corner.
**************************************************************************/
term_cursor_home()
{
	if ( mT_cursor_home != NULL )
		term_tputs( mT_cursor_home );		/* home */
	else
		term_pos( 0, 0 );
}
/**************************************************************************
* term_down_mul
*	Output a sequence to cause the cursor to move down "downrows" lines.
**************************************************************************/
term_down_mul( downrows )
	int	downrows;
{
	int	i;
	int	parm[ 2 ];			/* 1 used - must be >= 2 - %i */
	char	*string_parm[ 1 ];		/* not used */

	char	*p;
	if ( (downrows > 1) && (mT_parm_down_cursor != NULL) )
	{
		parm[ 0 ] = downrows;
		p = my_tparm( mT_parm_down_cursor, parm, string_parm, -1 );
		my_tputs( p, downrows );
		return;
	}
	for ( i = 0; i < downrows; i++ )
		term_down_one();
}
/**************************************************************************
* term_down_one
*	Output a sequence to cause the cursor to move down a line.
**************************************************************************/
term_down_one()
{
	term_tputs( mT_cursor_down );
}
/**************************************************************************
* term_up_mul
*	Output a sequence to cause the cursor on the terminal to move up
*	"uprows" lines.
**************************************************************************/
term_up_mul( uprows )
	int	uprows;
{
	int	parm[ 2 ];			/* 1 used - must be >= 2 - %i */
	char	*string_parm[ 1 ];		/* not used */
	char	*p;
	int	i;

	if ( ( (uprows > 1) && (mT_parm_up_cursor != NULL) )
	   || ( mT_cursor_up == (char *) 0 )
	   )
	{
		parm[ 0 ] = uprows;
		p = my_tparm( mT_parm_up_cursor, parm, string_parm, -1 );
		my_tputs( p, uprows );
		return;
	}
	for ( i = 0; i < uprows; i++ )
		term_tputs( mT_cursor_up );
}
/**************************************************************************
* term_left_mul
*	Output a sequence to cause the cursor on the terminal to move left
*	"leftrows" columns.
**************************************************************************/
term_left_mul( leftrows )
	int	leftrows;
{
	int	parm[ 2 ];			/* 1 used - must be >= 2 - %i */
	char	*string_parm[ 1 ];		/* not used */
	char	*p;
	int	i;

	if (  ( (leftrows > 1) && (mT_parm_left_cursor != NULL) )
	   || ( mT_cursor_left == (char *) 0 )
	   )
	{
		parm[ 0 ] = leftrows;
		p = my_tparm( mT_parm_left_cursor, parm, string_parm, -1 );
		my_tputs( p, leftrows );
		return;
	}
	for ( i = 0; i < leftrows; i++ )
		term_tputs( mT_cursor_left );
}
/**************************************************************************
* term_right_mul
*	Output a sequence to cause the cursor on the terminal to move right
*	"rightrows" columns.
**************************************************************************/
term_right_mul( rightrows )
	int	rightrows;
{
	char	*p;
	int	parm[ 2 ];			/* 1 used - must be >= 2 - %i */
	char	*string_parm[ 1 ];		/* not used */
	int	i;

	if (  ( (rightrows > 1) && (mT_parm_right_cursor != NULL) )
	   || ( mT_cursor_right == (char *) 0 )
	   )
	{
		parm[ 0 ] = rightrows;
		p = my_tparm( mT_parm_right_cursor, parm, string_parm, -1 );
		my_tputs( p, rightrows );
		return;
	}
	for ( i = 0; i < rightrows; i++ )
		term_tputs( mT_cursor_right );
}
/**************************************************************************
* term_enter_ca_mode
*	Output the terminal initialization string specified in the terminal
*	description file.
**************************************************************************/
term_enter_ca_mode()
{
	T_CA_MODE	*t_ca_mode_ptr;

	for (	t_ca_mode_ptr = T_enter_ca_mode_ptr;
		t_ca_mode_ptr != (T_CA_MODE *) 0;
		t_ca_mode_ptr = t_ca_mode_ptr->next )
	{
		term_tputs( t_ca_mode_ptr->t_ca_mode );
	}
	for (	t_ca_mode_ptr = T_enter_ca_mode_2_ptr;
		t_ca_mode_ptr != (T_CA_MODE *) 0;
		t_ca_mode_ptr = t_ca_mode_ptr->next )
	{
		term_tputs( t_ca_mode_ptr->t_ca_mode );
	}
}
/**************************************************************************
* term_exit_ca_mode
*	Output the terminal de-initialization string specified in the 
*	terminal description file.
**************************************************************************/
term_exit_ca_mode()
{
	T_CA_MODE	*t_ca_mode_ptr;

	for (	t_ca_mode_ptr = T_exit_ca_mode_ptr;
		t_ca_mode_ptr != (T_CA_MODE *) 0;
		t_ca_mode_ptr = t_ca_mode_ptr->next )
	{
		term_tputs( t_ca_mode_ptr->t_ca_mode );
	}
	for (	t_ca_mode_ptr = T_exit_ca_mode_2_ptr;
		t_ca_mode_ptr != (T_CA_MODE *) 0;
		t_ca_mode_ptr = t_ca_mode_ptr->next )
	{
		term_tputs( t_ca_mode_ptr->t_ca_mode );
	}
}
/**************************************************************************
* term_flash_screen
*	Output a sequence to cause the terminal's screen to flash.
**************************************************************************/
term_flash_screen()
{
	if ( T_flash_screen != NULL )
		term_tputs( T_flash_screen );
}
/**************************************************************************
* term_change_scroll_region
*	Output a sequence to cause the terminal's scrolling region to be
*	set to the "top_row" and "bot_row" line on the screen.
**************************************************************************/
term_change_scroll_region( top_row, bot_row )
	int	top_row;
	int	bot_row;
{
	char	*p;
	int	parm[ 2 ];
	char	*string_parm[ 1 ];		/* not used */

	if ( mT_change_scroll_region != NULL )
	{
		parm[ 0 ] = top_row;
		parm[ 1 ] = bot_row;
		p = my_tparm( mT_change_scroll_region, parm, string_parm, -1 );
		my_tputs( p, 1 );
		term_se_memory_unlock();
	}
	M_csr_top_row = top_row;
	M_csr_bot_row = bot_row;
}
/**************************************************************************
* term_init_change_scroll_region
*	Remember that the terminal has its scrolling region set to 
*	the lines "top_row" and "bot_row".
**************************************************************************/
term_init_change_scroll_region( top_row, bot_row )
	int	top_row;
	int	bot_row;
{
	M_csr_top_row = top_row;
	M_csr_bot_row = bot_row;
}
/**************************************************************************
* term_reload_scroll_region_normal
*	Set the terminal to full screen scroll regions.
**************************************************************************/
term_reload_scroll_region_normal()
{
	term_change_scroll_region( 0, Row_bottom_terminal );
}
/**************************************************************************
* term_se_scroll_region_normal
*	An event has occurred that has the side effect of setting the
*	terminals scrolling region to the default.
**************************************************************************/
term_se_scroll_region_normal()
{
	M_csr_top_row = 0;
	M_csr_bot_row = Row_bottom_terminal;
}
/**************************************************************************
* term_memory_lock
*	Output a sequence to cause an HP terminal to set a memory lock on
*	line number "row".
**************************************************************************/
term_memory_lock( row )
	int	row;
{
	if ( row < 0 )
	{
		term_memory_unlock();
		return;
	}
	if (  ( M_csr_top_row != 0 ) 
	   || ( M_csr_bot_row != Row_bottom_terminal ) )
	{
		term_beep();
	}
	if ( mT_memory_lock != NULL )
	{
		term_pos( row, 0 );
		my_tputs( mT_memory_lock, 1 );
	}
	M_memory_lock_row = row;
}
/**************************************************************************
* term_memory_unlock
*	Output a sequence to clear the memory lock on an HP terminal.
**************************************************************************/
term_memory_unlock()
{
	if ( mT_memory_unlock != NULL )
	{
		my_tputs( mT_memory_unlock, 1 );
	}
	M_memory_lock_row = -1;
}
/**************************************************************************
* term_init_memory_lock
*	Remember that the memory lock on an HP terminal is not set.
**************************************************************************/
term_init_memory_lock()
{
	M_memory_lock_row = -1;
}
/**************************************************************************
* term_se_memory_unlock
*	An event has occurred that has the side effect of clearing any
*	possible memory lock on an HP terminal.
**************************************************************************/
term_se_memory_unlock()
{
	M_memory_lock_row = -1;
}
/**************************************************************************
* make_out_nomagic
*	Construct as string that, when fed to the decoder, will be understood
*	as stopping the propogation of "magic cookie" (See termininfo
*	documentation) attributes.
**************************************************************************/
make_out_nomagic( string, max )
	char	*string;
	int	max;
{
	if ( T_nomagic != NULL )
	{
		nopad_strncpy( string, T_nomagic, max );
		return( 0 );
	}
	else
		return( -1 );
}
/**************************************************************************
* term_nomagic
*	Output a sequence which prevents the propogation of "magic cookie"
*	attributes past this position.
**************************************************************************/
term_nomagic()
{
	if ( T_nomagic != NULL )
		term_tputs( T_nomagic );
}
/**************************************************************************
* term_magic
*	Output a sequence which occupies a character position.  This is
*	generally used for terminals that have magic cookie attributes.
*	"magicno" indicates which sequence.
**************************************************************************/
term_magic( magicno )
	int magicno;
{
	char	*magicstr;

	if ( magicno < T_magicno )
	{
		magicstr = T_magic[ magicno ];
		if ( magicstr != NULL )
			term_tputs( magicstr );
	}
}
/**************************************************************************
* t_sync_auto_wrap
*	Synchronize the terminal, if necessary, to the auto wrap status
*	"auto_wrap_on".  This is the auto wrap of the cursor to the next line
*	when typing characters past the end of the current line.
**************************************************************************/
t_sync_auto_wrap( auto_wrap_on )
	int	auto_wrap_on;
{
	if ( auto_wrap_on != M_auto_wrap_on )
	{
		if ( auto_wrap_on )
			term_auto_wrap_on();
		else
			term_auto_wrap_off();
	}
}
/**************************************************************************
* term_auto_wrap_on
*	Output a sequence to cause the terminal to turn its auto wrap on.
**************************************************************************/
term_auto_wrap_on()
{
	if ( mT_auto_wrap_on != NULL )
	{
		term_tputs( mT_auto_wrap_on );
		M_auto_wrap_on = 1;
	}
}
/**************************************************************************
* term_auto_wrap_off
*	Output a sequence to cause the terminal to turn its auto wrap off.
**************************************************************************/
term_auto_wrap_off()
{
	if ( mT_auto_wrap_off != NULL )
	{
		term_tputs( mT_auto_wrap_off );
		M_auto_wrap_on = 0;
	}
}
/**************************************************************************
* term_se_auto_wrap_on
*	An event has occured that has the side effect of turning the
*	terminals auto wrap on.
**************************************************************************/
term_se_auto_wrap_on()
{
	M_auto_wrap_on = 1;
}
/**************************************************************************
* term_se_auto_wrap_off
*	An event has occured that has the side effect of turning the
*	terminals auto wrap off.
**************************************************************************/
term_se_auto_wrap_off()
{
	M_auto_wrap_on = 0;
}
/**************************************************************************
* term_columns_wide_on
*	Output a sequence to the terminal that will cause it to go to
*	its highest number of columns. E.G. 132 column mode.
**************************************************************************/
term_columns_wide_on()
{
	if ( T_columns_wide_on != NULL )
	{
		term_tputs( T_columns_wide_on );
		if (  ( M_columns_wide_on == 0 ) 
		   && ( F_columns_wide_switch_resets_scroll_region ) )
		{
			if ( F_columns_wide_switch_reload_scroll_region )
			{
				term_reload_scroll_region_normal();
				t_cursor();
			}
			else
				term_se_scroll_region_normal();
		}
		M_columns_wide_on = 1;
	}
	term_se_set_margins_normal();
}
/**************************************************************************
* term_columns_wide_off
*	Output a sequence to the terminal that will cause it to to to
*	its lowest number of columns E.g 80 column mode.
**************************************************************************/
term_columns_wide_off()
{
	if ( T_columns_wide_off != NULL )
	{
		term_tputs( T_columns_wide_off );
		if (  ( M_columns_wide_on ) 
		   && ( F_columns_wide_switch_resets_scroll_region ) )
		{
			if ( F_columns_wide_switch_reload_scroll_region )
			{
				term_reload_scroll_region_normal();
				t_cursor();
			}
			else
				term_se_scroll_region_normal();
		}
		M_columns_wide_on = 0;
	}
	term_se_set_margins_normal();
}
/**************************************************************************
* term_is_columns_wide_on
*	Return 1 if the terminal is in 132 column mode. 0 otherwise.
**************************************************************************/
term_is_columns_wide_on()
{
	return( M_columns_wide_on );
}
/**************************************************************************
* term_pad
*	Output the padding specified in the padding specification "padstr".
**************************************************************************/
term_pad( padstr )
	char	*padstr;
{
	if ( padstr != NULL )
		term_tputs( padstr );
}
