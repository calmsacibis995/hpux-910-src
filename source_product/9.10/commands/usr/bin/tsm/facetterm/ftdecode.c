/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: ftdecode.c,v 70.1 92/03/09 15:43:00 ssa Exp $ */
/**************************************************************************
* ftdecode.c
*	Decode outgoing character stream.
**************************************************************************/
#include "ftchar.h"
#include "ftproc.h"
#include "stdio.h"

#include "features.h"
#include "extra.h"
#include "wins.h"
#include "ftterm.h"

#include "decode.h"
#include "options.h"

#include "max_buff.h"

#include "ftwindow.h"

typedef struct 
{
	UNCHAR 	lastchar; 		/* last char received for rep command */
	int	decode_type;		/* flag for template decode */
	int	buffno;			/* number of chars in current sequence*/
	UNCHAR	buff[ MAX_BUFF ];	/* chars of current sequence */
} FT_DECODE;

int	Dec_parms_valid;		/* number of parms on current sequence*/
int	Dec_parm[ MAX_PARM ];	/* values of current sequence */
int	Dec_string_parms_valid;
UNCHAR	*Dec_string_parm[ MAX_PARM ];
					/* values of current sequence- strings*/
int	Dec_length[ MAX_PARM ];	/* storage for length for fixed string*/

FT_DECODE *Decode[ TOPWIN ];
FT_DECODE *Outdecode;			/* decode structure for Outwin */

#include "person.h"

/**************************************************************************
* dec_linefeed
*	DECODE module for "linefeed".
**************************************************************************/
dec_linefeed()
{
	fct_linefeed();
}
/**************************************************************************
* inst_linefeed
*	INSTALL module for "linefeed".
**************************************************************************/
inst_linefeed( str )
	char	*str;
{
	dec_install( "linefeed", (UNCHAR *) str, dec_linefeed, 0, 0,
	(char *) 0 );
}
/**************************************************************************
* dec_carriage_return
*	DECODE module for "carriage_return".
**************************************************************************/
dec_carriage_return()
{
	fct_carriage_return();
}
/**************************************************************************
* inst_carriage_return
*	INSTALL module for "carriage_return".
**************************************************************************/
inst_carriage_return( str )
	char	*str;
{
	dec_install( "carriage_return", (UNCHAR *) str,
			dec_carriage_return, 0, 0,
			(char *) 0 );
}
/**************************************************************************
* dec_tab
*	DECODE module for "tab".
**************************************************************************/
dec_tab( not_control_i )
	int	not_control_i;
{
	fct_tab( not_control_i );
}
/**************************************************************************
* inst_tab
*	INSTALL module for "tab".
**************************************************************************/
inst_tab( str, not_control_i )
	char	*str;
	int	not_control_i;
{
	dec_install( "tab", (UNCHAR *) str, dec_tab, not_control_i, 0,
	(char *) 0 );
}
/**************************************************************************
* dec_back_tab
*	DECODE module for "back_tab".
**************************************************************************/
dec_back_tab()
{
	fct_back_tab();
}
/**************************************************************************
* inst_back_tab
*	INSTALL module for "back_tab".
**************************************************************************/
inst_back_tab( str )
	char	*str;
{
	dec_install( "back_tab", (UNCHAR *) str, dec_back_tab, 0, 0,
	(char *) 0 );
}
/**************************************************************************
* dec_backspace
*	DECODE module for "backspace".
**************************************************************************/
dec_backspace()
{
	fct_backspace();
}
/**************************************************************************
* inst_backspace
*	INSTALL module for "backspace".
**************************************************************************/
inst_backspace( str )
	char	*str;
{
	dec_install( "backspace", (UNCHAR *) str, dec_backspace, 0, 0,
	(char *) 0 );
}
/**************************************************************************
* dec_beep
*	DECODE module for "\007".
**************************************************************************/
dec_beep()
{
	fct_beep();
}
/**************************************************************************
* inst_beep
*	INSTALL module for "\007".
**************************************************************************/
inst_beep( str )
	char	*str;
{
	dec_install( "beep", (UNCHAR *) str, dec_beep, 0, 0,
	(char *) 0 );
}
/**************************************************************************
* dec_cursor_address
*	DECODE module for "cursor_address".
**************************************************************************/
/*ARGSUSED*/
dec_cursor_address( func_parm, parm_ptr, parms_valid, parm )
	int	func_parm;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
{
	int	row;
	int	col;

	if ( parms_valid & 1 )
		row = parm[ 0 ];
	else
		row = 0;
	if ( parms_valid & 2 )
		col = parm[ 1 ];
	else
		col = 0;
	fct_cursor_address( row, col );
}
/**************************************************************************
* inst_cursor_address
*	INSTALL module for "cursor_address".
**************************************************************************/
inst_cursor_address( str )
	char	*str;
{
	dec_install( "cursor_address", (UNCHAR *) str, dec_cursor_address, 
		0, CURSOR_OPTION,
		(char *) 0 );
}
/**************************************************************************
* inst_cursor_address_wide
*	INSTALL module for "cursor_address_wide".
**************************************************************************/
inst_cursor_address_wide( str )
	char	*str;
{
	dec_install( "cursor_address_wide", (UNCHAR *) str, dec_cursor_address, 
		0, CURSOR_OPTION,
		(char *) 0 );
}
/**************************************************************************
* dec_row_address
*	DECODE module for "row_address".
**************************************************************************/
/*ARGSUSED*/
dec_row_address( func_parm, parm_ptr, parms_valid, parm )
	int	func_parm;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
{
	int	row;

	if ( parms_valid & 1 )
		row = parm[ 0 ];
	else
		row = 0;
	fct_row_address( row );
}
/**************************************************************************
* inst_row_address
*	INSTALL module for "row_address".
**************************************************************************/
inst_row_address( str )
	char	*str;
{
	dec_install( "row_address", (UNCHAR *) str, dec_row_address, 
		0, CURSOR_OPTION,
		(char *) 0 );
}
/**************************************************************************
* dec_row_address_addsvp
*	DECODE module for "row_address_addsvp".
*	This is special purpose code for the ADDS VIEWPOINT only.
*	The row number is encoded in the lower 5 bits of the byte.
*	Various software uses various high order bits to make characters.
**************************************************************************/
/*ARGSUSED*/
dec_row_address_addsvp( func_parm, parm_ptr, parms_valid, parm )
	int	func_parm;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
{
	int	row_addsvp;
	int	row;

	if ( parms_valid & 1 )
		row_addsvp = parm[ 0 ];
	else
		row_addsvp = 0;
	row = row_addsvp & 0x1F;
	fct_row_address( row );
}
/**************************************************************************
* inst_row_address_addsvp
*	INSTALL module for "row_address_addsvp".
**************************************************************************/
inst_row_address_addsvp( str )
	char	*str;
{
	dec_install( "row_address_addsvp", (UNCHAR *) str,
		dec_row_address_addsvp, 
		0, CURSOR_OPTION,
		(char *) 0 );
}
/**************************************************************************
* dec_column_address
*	DECODE module for "column_address".
**************************************************************************/
/*ARGSUSED*/
dec_column_address( func_parm, parm_ptr, parms_valid, parm )
	int	func_parm;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
{
	int	col;

	if ( parms_valid & 1 )
		col = parm[ 0 ];
	else
		col = 0;
	fct_column_address( col );
}
/**************************************************************************
* inst_column_address
*	INSTALL module for "column_address".
**************************************************************************/
inst_column_address( str )
	char	*str;
{
	dec_install( "column_address", (UNCHAR *) str, dec_column_address, 
		0, CURSOR_OPTION,
		(char *) 0 );
}
/**************************************************************************
* dec_column_address_addsvp
*	DECODE module for "column_address_addsvp".
*	This is special purpose code for the ADDS VIEWPOINT only.
*	The column number is encoded with the tens digit of the decimal
*	column number as binary in the top 4 bits and the ones digit in
*	the bottom 4 bits.
**************************************************************************/
/*ARGSUSED*/
dec_column_address_addsvp( func_parm, parm_ptr, parms_valid, parm )
	int	func_parm;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
{
	int	col_addsvp;
	int	col;
	int	tens;

	if ( parms_valid & 1 )
		col_addsvp = parm[ 0 ];
	else
		col_addsvp = 0;
	tens = col_addsvp / 16;
	col = ( tens * 10 ) + ( col_addsvp - ( tens * 16 ) );
	fct_column_address( col );
}
/**************************************************************************
* inst_column_address_addsvp
*	INSTALL module for "column_address_addsvp".
**************************************************************************/
inst_column_address_addsvp( str )
	char	*str;
{
	dec_install( "column_address_addsvp", (UNCHAR *) str, 
		dec_column_address_addsvp, 
		0, CURSOR_OPTION,
		(char *) 0 );
}
/**************************************************************************
* dec_column_address_parm_up_cursor
*	DECODE module for "column_address_parm_up_cursor".
*	HP command that has absolute column number but relative row number.
**************************************************************************/
/*ARGSUSED*/
dec_column_address_parm_up_cursor( func_parm, parm_ptr, parms_valid, parm )
	int	func_parm;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
{
	int	col;
	int	rows;

	if ( parms_valid & 1 )
		col = parm[ 0 ];
	else
		col = 0;
	if ( parms_valid & 2 )
		rows = parm[ 1 ];
	else
		rows = 1;
	fct_column_address( col );
	fct_cursor_up( rows );
}
/**************************************************************************
* inst_column_address_parm_up_cursor
*	INSTALL module for "column_address_parm_up_cursor".
**************************************************************************/
inst_column_address_parm_up_cursor( str )
	char	*str;
{
	dec_install( "column_address_parm_up_cursor", (UNCHAR *) str, 
		dec_column_address_parm_up_cursor, 
		0, CURSOR_OPTION,
		(char *) 0 );
}
/**************************************************************************
* dec_column_address_parm_down_cursor
*	DECODE module for "column_address_parm_down_cursor".
*	HP command that has absolute column number but relative row number.
**************************************************************************/
/*ARGSUSED*/
dec_column_address_parm_down_cursor( func_parm, parm_ptr, parms_valid, parm )
	int	func_parm;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
{
	int	col;
	int	rows;

	if ( parms_valid & 1 )
		col = parm[ 0 ];
	else
		col = 0;
	if ( parms_valid & 2 )
		rows = parm[ 1 ];
	else
		rows = 1;
	fct_column_address( col );
	fct_cursor_down( rows );
}
/**************************************************************************
* inst_column_address_parm_down_cursor
*	INSTALL module for "column_address_parm_down_cursor".
**************************************************************************/
inst_column_address_parm_down_cursor( str )
	char	*str;
{
	dec_install( "column_address_parm_down_cursor", (UNCHAR *) str, 
		dec_column_address_parm_down_cursor, 
		0, CURSOR_OPTION,
		(char *) 0 );
}
/**************************************************************************
* dec_new_line
*	DECODE module for "new_line".
**************************************************************************/
dec_new_line()
{
	fct_carriage_return();
	fct_linefeed();
}
/**************************************************************************
* inst_new_line
*	INSTALL module for "new_line".
**************************************************************************/
inst_new_line( str )
	char	*str;
{
	dec_install( "new_line", (UNCHAR *) str, dec_new_line, 0, 0,
	(char *) 0 );
}
/**************************************************************************
* dec_cursor_home
*	DECODE module for "cursor_home".
**************************************************************************/
dec_cursor_home()
{
	fct_cursor_address( 0, 0 );
}
/**************************************************************************
* inst_cursor_home
*	INSTALL module for "cursor_home".
**************************************************************************/
inst_cursor_home( str )
	char	*str;
{
	dec_install( "cursor_home", (UNCHAR *) str, dec_cursor_home, 0, 0,
	(char *) 0 );
}
/**************************************************************************
* dec_cursor_up
*	DECODE module for "cursor_up".
**************************************************************************/
/*ARGSUSED*/
dec_cursor_up( func_parm, parm_ptr, parms_valid, parm )
	int	func_parm;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
{
	int	rows;

	if ( parms_valid & 1 )
		rows = parm[ 0 ];
	else
		rows = 1;
	fct_cursor_up( rows );
}
/**************************************************************************
* inst_cursor_up
*	INSTALL module for "cursor_up".
**************************************************************************/
inst_cursor_up( str )
	char	*str;
{
	dec_install( "cursor_up", (UNCHAR *) str, dec_cursor_up, 0, 0,
	(char *) 0 );
}
/**************************************************************************
* inst_parm_up_cursor
*	INSTALL module for "parm_up_cursor".
**************************************************************************/
inst_parm_up_cursor( str )
	char	*str;
{
	dec_install( "parm_up_cursor", (UNCHAR *) str, dec_cursor_up, 0,
			CURSOR_OPTION,
			(char *) 0 );
}
/**************************************************************************
* dec_cursor_down
*	DECODE module for "cursor_down".
**************************************************************************/
/*ARGSUSED*/
dec_cursor_down( func_parm, parm_ptr, parms_valid, parm )
	int	func_parm;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
{
	int	rows;

	if ( parms_valid & 1 )
		rows = parm[ 0 ];
	else
		rows = 1;
	fct_cursor_down( rows );
}
/**************************************************************************
* inst_cursor_down
*	INSTALL module for "cursor_down".
**************************************************************************/
inst_cursor_down( str )
	char	*str;
{
	dec_install( "cursor_down", (UNCHAR *) str, dec_cursor_down, 0, 0,
	(char *) 0 );
}
/**************************************************************************
* inst_parm_down_cursor
*	INSTALL module for "parm_down_cursor".
**************************************************************************/
inst_parm_down_cursor( str )
	char	*str;
{
	dec_install( "parm_down_cursor", (UNCHAR *) str, dec_cursor_down, 0,
			 CURSOR_OPTION,
			 (char *) 0 );
}
/**************************************************************************
* dec_cursor_right
*	DECODE module for "cursor_right".
**************************************************************************/
/*ARGSUSED*/
dec_cursor_right( func_parm, parm_ptr, parms_valid, parm )
	int	func_parm;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
{
	int	cols;

	if ( parms_valid & 1 )
		cols = parm[ 0 ];
	else
		cols = 1;
	fct_cursor_right( cols );
}
/**************************************************************************
* inst_cursor_right
*	INSTALL module for "cursor_right".
**************************************************************************/
inst_cursor_right( str )
	char	*str;
{
	dec_install( "cursor_right", (UNCHAR *) str, dec_cursor_right, 0, 0,
	(char *) 0 );
}
/**************************************************************************
* inst_parm_right_cursor
*	INSTALL module for "right_cursor".
**************************************************************************/
inst_parm_right_cursor( str )
	char	*str;
{
	dec_install( "parm_right_cursor", (UNCHAR *) str, dec_cursor_right, 0, 
			CURSOR_OPTION,
			(char *) 0 );
}
/**************************************************************************
* dec_cursor_left
*	DECODE module for "cursor_left".
**************************************************************************/
/*ARGSUSED*/
dec_cursor_left( func_parm, parm_ptr, parms_valid, parm )
	int	func_parm;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
{
	int	cols;
	
	if ( parms_valid & 1 )
		cols = parm[ 0 ];
	else
		cols = 1;
	fct_cursor_left( cols );
}
/**************************************************************************
* inst_cursor_left
*	INSTALL module for "cursor_left".
**************************************************************************/
inst_cursor_left( str )
	char	*str;
{
	dec_install( "cursor_left", (UNCHAR *) str, dec_cursor_left, 0, 0,
	(char *) 0 );
}
/**************************************************************************
* inst_parm_left_cursor
*	INSTALL module for "parm_left_cursor".
**************************************************************************/
inst_parm_left_cursor( str )
	char	*str;
{
	dec_install( "parm_left_cursor", (UNCHAR *) str, dec_cursor_left, 0, 
			CURSOR_OPTION,
			(char *) 0 );
}
/**************************************************************************
* dec_cursor_to_ll
*	DECODE module for "cursor_to_ll".
**************************************************************************/
/*ARGSUSED*/
dec_cursor_to_ll( func_parm, parm_ptr, parms_valid, parm )
	int	func_parm;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
{
	fct_cursor_address( Outwin->display_row_bottom, 0 );
}
/**************************************************************************
* inst_cursor_to_ll
*	INSTALL module for "cursor_to_ll".
**************************************************************************/
inst_cursor_to_ll( str )
	char	*str;
{
	dec_install( "cursor_to_ll", (UNCHAR *) str, dec_cursor_to_ll, 
		0, CURSOR_OPTION,
		(char *) 0 );
}
/**************************************************************************
* dec_insert_character
*	DECODE module for "insert_character".
**************************************************************************/
/*ARGSUSED*/
dec_insert_character( func_parm, parm_ptr, parms_valid, parm )
	int	func_parm;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
{
	int	cols;

	if ( parms_valid & 1 )
		cols = parm[ 0 ];
	else
		cols = 1;
	fct_insert_character( cols );
}
/**************************************************************************
* inst_insert_character
*	INSTALL module for "insert_character".
**************************************************************************/
inst_insert_character( str )
	char	*str;
{
	dec_install( "insert_character", (UNCHAR *) str, dec_insert_character, 
		0, 0,
		(char *) 0 );
}
/**************************************************************************
* inst_parm_ich
*	INSTALL module for "parm_ich".
*	parm insert character
**************************************************************************/
inst_parm_ich( str )
	char	*str;
{
	dec_install( "parm_ich", (UNCHAR *) str, dec_insert_character, 
		0, CURSOR_OPTION,
		(char *) 0 );
}
/**************************************************************************
* dec_enter_insert_mode
*	DECODE module for "enter_insert_mode".
**************************************************************************/
dec_enter_insert_mode()
{
	fct_enter_insert_mode();
}
/**************************************************************************
* inst_enter_insert_mode
*	INSTALL module for "enter_insert_mode".
**************************************************************************/
inst_enter_insert_mode( str )
	char	*str;
{
	dec_install( "enter_insert_mode", (UNCHAR *) str, dec_enter_insert_mode,
		0, 0,
		(char *) 0 );
}
/**************************************************************************
* dec_exit_insert_mode
*	DECODE module for "exit_insert_mode".
**************************************************************************/
dec_exit_insert_mode()
{
	fct_exit_insert_mode();
}
/**************************************************************************
* inst_exit_insert_mode
*	INSTALL module for "exit_insert_mode".
**************************************************************************/
inst_exit_insert_mode( str )
	char	*str;
{
	dec_install( "exit_insert_mode", (UNCHAR *) str, dec_exit_insert_mode, 
		0, 0,
		(char *) 0 );
}
/**************************************************************************
* dec_delete_character
*	DECODE module for "delete_character".
**************************************************************************/
/*ARGSUSED*/
dec_delete_character( func_parm, parm_ptr, parms_valid, parm )
	int	func_parm;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
{
	int	cols;

	if ( parms_valid & 1 )
		cols = parm[ 0 ];
	else
		cols = 1;
	fct_delete_character( cols );
}
/**************************************************************************
* inst_delete_character
*	INSTALL module for "delete_character".
**************************************************************************/
inst_delete_character( str )
	char	*str;
{
	dec_install( "delete_character", (UNCHAR *) str, dec_delete_character, 
		0, 0,
		(char *) 0 );
}
/**************************************************************************
* inst_parm_delete_character
*	INSTALL module for "parm_delete_character".
**************************************************************************/
inst_parm_delete_character( str )
	char	*str;
{
	dec_install( "parm_delete_character", (UNCHAR *) str,
		dec_delete_character, 0, CURSOR_OPTION,
		(char *) 0 );
}
/**************************************************************************
* dec_insert_line
*	DECODE module for "insert_line".
**************************************************************************/
/*ARGSUSED*/
dec_insert_line( func_parm, parm_ptr, parms_valid, parm )
	int	func_parm;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
{
	int	rows;

	if ( parms_valid & 1 )
		rows = parm[ 0 ];
	else
		rows = 1;
	fct_insert_line( rows );
}
/**************************************************************************
* inst_insert_line
*	INSTALL module for "insert_line".
**************************************************************************/
inst_insert_line( str )
	char	*str;
{
	dec_install( "insert_line", (UNCHAR *) str, dec_insert_line, 0, 0,
	(char *) 0 );
}
/**************************************************************************
* inst_parm_insert_line
*	INSTALL module for "parm_insert_line".
**************************************************************************/
inst_parm_insert_line( str )
	char	*str;
{
	dec_install( "parm_insert_line", (UNCHAR *) str,
		dec_insert_line, 0, CURSOR_OPTION,
		(char *) 0 );
}
/**************************************************************************
* dec_delete_line
*	DECODE module for "delete_line".
**************************************************************************/
/*ARGSUSED*/
dec_delete_line( func_parm, parm_ptr, parms_valid, parm )
	int	func_parm;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
{
	int	rows;

	if ( parms_valid & 1 )
		rows = parm[ 0 ];
	else
		rows = 1;
	fct_delete_line( rows );
}
/**************************************************************************
* inst_delete_line
*	INSTALL module for "delete_line".
**************************************************************************/
inst_delete_line( str )
	char	*str;
{
	dec_install( "delete_line", (UNCHAR *) str, dec_delete_line, 0, 0,
	(char *) 0 );
}
/**************************************************************************
* inst_parm_delete_line
*	INSTALL module for "parm_delete_line".
**************************************************************************/
inst_parm_delete_line( str )
	char	*str;
{
	dec_install( "parm_delete_line", (UNCHAR *) str,
		dec_delete_line, 0, CURSOR_OPTION,
		(char *) 0 );
}
/**************************************************************************
* dec_cursor_home_down
*	DECODE module for "cursor_home_down".
**************************************************************************/
/*ARGSUSED*/
dec_cursor_home_down( func_parm, parm_ptr, parms_valid, parm )
	int	func_parm;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
{
	fct_cursor_home_down();
}
/**************************************************************************
* inst_cursor_home_down
*	INSTALL module for "cursor_home_down".
**************************************************************************/
inst_cursor_home_down( str )
	char	*str;
{
	dec_install( "cursor_home_down", (UNCHAR *) str, dec_cursor_home_down, 
		0, 0,
		(char *) 0 );
}
/**************************************************************************
* dec_scroll_forward
*	DECODE module for "scroll_forward".
**************************************************************************/
/*ARGSUSED*/
dec_scroll_forward( func_parm, parm_ptr, parms_valid, parm )
	int	func_parm;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
{
	int	rows;

	if ( parms_valid & 1 )
		rows = parm[ 0 ];
	else
		rows = 1;
	fct_scroll_forward( rows );
}
/**************************************************************************
* inst_scroll_forward
*	INSTALL module for "scroll_forward".
**************************************************************************/
inst_scroll_forward( str )
	char	*str;
{
	dec_install( "scroll_forward", (UNCHAR *) str, dec_scroll_forward, 
		0, 0,
		(char *) 0 );
}
/**************************************************************************
* inst_parm_index
*	INSTALL module for "parm_index".
**************************************************************************/
inst_parm_index( str )
	char	*str;
{
	dec_install( "parm_index", (UNCHAR *) str, dec_scroll_forward, 
		0, CURSOR_OPTION,
		(char *) 0 );
}
/**************************************************************************
* dec_scroll_reverse
*	DECODE module for "scroll_reverse".
**************************************************************************/
/*ARGSUSED*/
dec_scroll_reverse( func_parm, parm_ptr, parms_valid, parm )
	int	func_parm;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
{
	int	rows;

	if ( parms_valid & 1 )
		rows = parm[ 0 ];
	else
		rows = 1;
	fct_scroll_reverse( rows );
}
/**************************************************************************
* inst_scroll_reverse
*	INSTALL module for "scroll_reverse".
**************************************************************************/
inst_scroll_reverse( str )
	char	*str;
{
	dec_install( "scroll_reverse", (UNCHAR *) str, dec_scroll_reverse, 
		0, 0,
		(char *) 0 );
}
/**************************************************************************
* inst_parm_rindex
*	INSTALL module for "parm_rindex".
**************************************************************************/
inst_parm_rindex( str )
	char	*str;
{
	dec_install( "parm_rindex", (UNCHAR *) str, dec_scroll_reverse, 
		0, CURSOR_OPTION,
		(char *) 0 );
}
/**************************************************************************
* dec_flash_screen
*	DECODE module for "flash_screen".
**************************************************************************/
dec_flash_screen()
{
	fct_flash_screen();
}
/**************************************************************************
* inst_flash_screen
*	INSTALL module for "flash_screen".
**************************************************************************/
inst_flash_screen( str )
	char	*str;
{
	dec_install( "flash_screen", (UNCHAR *) str, dec_flash_screen, 0, 0,
	(char *) 0 );
}
/**************************************************************************
* dec_change_scroll_region
*	DECODE module for "change_scroll_region".
**************************************************************************/
#define SET_SCROLL_REGION_BOTTOM_TO_MAX 1000
/*ARGSUSED*/
dec_change_scroll_region( func_parm, parm_ptr, parms_valid, parm )
	int	func_parm;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
{
	int	top_row;
	int	bot_row;

	if ( parms_valid & 1 )
		top_row = parm[ 0 ];
	else
		top_row = 0;
	if ( parms_valid & 2 )
		bot_row = parm[ 1 ];
	else
		bot_row = SET_SCROLL_REGION_BOTTOM_TO_MAX;
	fct_change_scroll_region( top_row, bot_row );
}
/**************************************************************************
* inst_change_scroll_region
*	INSTALL module for "change_scroll_region".
**************************************************************************/
inst_change_scroll_region( str )
	char	*str;
{
	dec_install( "change_scroll_region", (UNCHAR *) str,
		dec_change_scroll_region, 0, CURSOR_OPTION,
		(char *) 0 );
}
/**************************************************************************
* dec_memory_lock
*	DECODE module for "memory_lock".
**************************************************************************/
dec_memory_lock()
{
	fct_memory_lock();
}
/**************************************************************************
* inst_memory_lock
*	INSTALL module for "memory_lock".
**************************************************************************/
inst_memory_lock( str )
	char	*str;
{
	dec_install( "memory_lock", (UNCHAR *) str, dec_memory_lock, 0, 0,
	(char *) 0 );
}
/**************************************************************************
* dec_memory_unlock
*	DECODE module for "memory_unlock".
**************************************************************************/
dec_memory_unlock()
{
	fct_memory_unlock();
}
/**************************************************************************
* inst_memory_unlock
*	INSTALL module for "memory_unlock".
**************************************************************************/
inst_memory_unlock( str )
	char	*str;
{
	dec_install( "unmemory_lock", (UNCHAR *) str, dec_memory_unlock, 0, 0,
	(char *) 0 );
}
/**************************************************************************
* dec_save_cursor
*	DECODE module for "save_cursor".
**************************************************************************/
dec_save_cursor()
{
	fct_save_cursor();
}
/**************************************************************************
* inst_save_cursor
*	INSTALL module for "save_cursor".
**************************************************************************/
inst_save_cursor( str )
	char	*str;
{
	dec_install( "save_cursor", (UNCHAR *) str, dec_save_cursor, 0, 0,
	(char *) 0 );
}
/**************************************************************************
* dec_restore_cursor
*	DECODE module for "restore_cursor".
**************************************************************************/
dec_restore_cursor()
{
	fct_restore_cursor();
}
/**************************************************************************
* inst_restore_cursor
*	INSTALL module for "restore_cursor".
**************************************************************************/
inst_restore_cursor( str )
	char	*str;
{
	dec_install( "restore_cursor", (UNCHAR *) str, dec_restore_cursor,
			 0, 0,
			 (char *) 0 );
}
/**************************************************************************
* dec_nomagic
*	DECODE module for "nomagic".
*	Turns of "magic cookie" propogating attributes.
**************************************************************************/
dec_nomagic()
{
	fct_nomagic();
}
/**************************************************************************
* inst_nomagic
*	INSTALL module for "nomagic".
**************************************************************************/
inst_nomagic( str )
	char	*str;
{
	dec_install( "nomagic", (UNCHAR *) str, dec_nomagic, 0, 0,
	(char *) 0 );
}
/**************************************************************************
* dec_magic
*	DECODE module for "magic".
*	"magic cookie" attributes.
**************************************************************************/
/*ARGSUSED*/
dec_magic( magicno, parm_ptr )
	int	magicno;
	char	*parm_ptr;
{
	fct_magic( magicno );
}
/**************************************************************************
* inst_magic
*	INSTALL module for "magic".
**************************************************************************/
inst_magic( str, magicno )
	char	*str;
	int	magicno;
{
	dec_install( "magic", (UNCHAR *) str, dec_magic, magicno, 0,
	(char *) 0 );
}
/**************************************************************************
* dec_auto_wrap_on
*	DECODE module for "auto_wrap_on".
**************************************************************************/
dec_auto_wrap_on()
{
	fct_auto_wrap_on();
}
/**************************************************************************
* inst_auto_wrap_on
*	INSTALL module for "auto_wrap_on".
**************************************************************************/
inst_auto_wrap_on( str )
	char	*str;
{
	dec_install( "auto_wrap_on", (UNCHAR *) str, dec_auto_wrap_on, 0, 0,
	(char *) 0 );
}
/**************************************************************************
* dec_auto_wrap_off
*	DECODE module for "auto_wrap_off".
**************************************************************************/
dec_auto_wrap_off()
{
	fct_auto_wrap_off();
}
/**************************************************************************
* inst_auto_wrap_off
*	INSTALL module for "auto_wrap_off".
**************************************************************************/
inst_auto_wrap_off( str )
	char	*str;
{
	dec_install( "auto_wrap_off", (UNCHAR *) str, dec_auto_wrap_off, 0, 0,
	(char *) 0 );
}
/**************************************************************************
* dec_columns_wide_on
*	DECODE module for "columns_wide_on".
**************************************************************************/
dec_columns_wide_on()
{
	fct_columns_wide_on();
}
/**************************************************************************
* inst_columns_wide_on
*	INSTALL module for "columns_wide_on".
**************************************************************************/
inst_columns_wide_on( str )
	char	*str;
{
	dec_install( "columns_wide_on", (UNCHAR *) str, 
			dec_columns_wide_on, 0, 0,
			(char *) 0 );
}
/**************************************************************************
* dec_columns_wide_off
*	DECODE module for "columns_wide_off".
**************************************************************************/
dec_columns_wide_off()
{
	fct_columns_wide_off();
}
/**************************************************************************
* inst_columns_wide_off
*	INSTALL module for "columns_wide_off".
**************************************************************************/
inst_columns_wide_off( str )
	char	*str;
{
	dec_install( "columns_wide_off", (UNCHAR *) str,
			dec_columns_wide_off, 0, 0,
			(char *) 0 );
}
/**************************************************************************
* dec_ignore
*	DECODE module for "dec_ignore".
**************************************************************************/
dec_ignore()
{
}
/**************************************************************************
* inst_ignore
*	INSTALL module for "ignore".
**************************************************************************/
inst_ignore( str, func_parm, option )
	char	*str;
	int	func_parm;
	int	option;		/* 0 = normal  1 = contains % strings */
{
	dec_install( "ignore", (UNCHAR *) str, dec_ignore, func_parm, option,
	(char *) 0 );
}
/**************************************************************************
* dec_not_imp
*	DECODE module for "not_imp".
*	not implemented - only beep to screen - error file if present.
**************************************************************************/
dec_not_imp()
{
	if ( Opt_error_ignore )
	{
	}
	else if ( Opt_error_pass )
	{
	}
	else
	{
		term_beep();
		term_outgo();
		error_record( Outdecode->buff, Outdecode->buffno );
		term_beep();
	}
}
/**************************************************************************
* inst_not_imp
*	INSTALL module for "not_imp".
**************************************************************************/
inst_not_imp( str, func_parm, option )
	char	*str;
	int	func_parm;
	int	option;		/* 0 = normal  1 = contains % strings */
{
	dec_install( "not_imp", (UNCHAR *) str, dec_not_imp,
			func_parm, option,
			(char *) 0 );
}
/**************************************************************************
* dec_pass
*	DECODE module for "pass".
*	Send through to terminal without recording.
**************************************************************************/
/*ARGSUSED*/
dec_pass( passno, padstring )
	int	passno;
	char	*padstring;
{
	int	len;
	len = squeeze_nulls( (char *) Outdecode->buff, Outdecode->buffno );
	term_write( (char *) Outdecode->buff, len );
	term_pad( padstring );
}
/**************************************************************************
* inst_pass
*	INSTALL module for "pass".
**************************************************************************/
inst_pass( str, pad )
	char	*str;
	char	*pad;
{
	dec_install( "pass", (UNCHAR *) str, dec_pass, 0, CURSOR_OPTION,
	pad );
}
/**************************************************************************
* dec_pass_current
*	DECODE module for "pass_current".
*	Same as pass but effective only if the current window - ignore
*	otherwise.
**************************************************************************/
/*ARGSUSED*/
dec_pass_current( passno, padstring )
	int	passno;
	char	*padstring;
{
	if ( outwin_is_curwin() )
	{
		int	len;

		len = squeeze_nulls( (char *) Outdecode->buff,
					      Outdecode->buffno );
		term_write( (char *) Outdecode->buff, len );
		term_pad( padstring );
	}
}
/**************************************************************************
* inst_pass_current
*	INSTALL module for "pass_current".
**************************************************************************/
inst_pass_current( str, pad )
	char	*str;
	char	*pad;
{
	dec_install( "pass_current", (UNCHAR *) str, 
		     dec_pass_current, 0, CURSOR_OPTION,
		     pad );
}
/**************************************************************************
* dec_pass_same_personality
*	DECODE module for "pass_same_personality".
*	Pass only if the terminal is in the same personality as this window
*	ignore otherwise.
**************************************************************************/
/*ARGSUSED*/
dec_pass_same_personality( passno, padstring )
	int	passno;
	char	*padstring;
{
	if ( Outwin->personality == M_pe )
	{
		int	len;

		len = squeeze_nulls( (char *) Outdecode->buff,
					      Outdecode->buffno );
		term_write( (char *) Outdecode->buff, len );
		term_pad( padstring );
	}
}
/**************************************************************************
* inst_pass_same_personality
*	INSTALL module for "pass_same_personality".
**************************************************************************/
inst_pass_same_personality( str, pad )
	char	*str;
	char	*pad;
{
	dec_install( "pass_same_personality", (UNCHAR *) str, 
		     dec_pass_same_personality, 0, CURSOR_OPTION,
		     pad );
}
/**************************************************************************
* squeeze_nulls
*	Remove nulls in the first "len" characters of buff.
*	Return resulting length.
**************************************************************************/
squeeze_nulls( buff, len )
	char	*buff;
	int	len;
{
	int	count;
	char	*s;
	char	*d;
	char	c;
	int	i;

	count = 0;
	s = buff;
	d = buff;
	for ( i = 0; i < len; i++ )
	{
		c = *s++;
		if ( c != '\0' )
		{
			*d++ = c;
			count++;
		}
	}
	return( count );
}
/**************************************************************************
* dec_perwindow
*	DECODE module for "perwindow".
*	Remember the last sequence of this pattern for each window.
**************************************************************************/
/*ARGSUSED*/
dec_perwindow( perwindowno, parm_ptr, parms_valid, parm, 
					string_parm, string_parms_valid )
	int	perwindowno;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
	UNCHAR	*string_parm[];
	int	string_parms_valid;
{
	int	len;
	UNCHAR	*s;

	len = squeeze_nulls( (char *) Outdecode->buff, Outdecode->buffno );
	if ( string_parms_valid & 1 )
		s = string_parm[ 0 ];
	else
		s = (UNCHAR *) 0;
	fct_perwindow( perwindowno, (char *) Outdecode->buff, len, s );
}
/**************************************************************************
* inst_perwindow
*	INSTALL module for "perwindow".
**************************************************************************/
inst_perwindow( str, perwindowno )
	char	*str;
	int	perwindowno;
{
	dec_install( "perwindow", (UNCHAR *) str, dec_perwindow, 
		perwindowno, CURSOR_OPTION,
		(char *) 0 );
}
/* =================== decoder ========================================== */
struct ft_seq
{
	int	code;
	struct ft_seq	*next;
	struct ft_seq	*also;
	int	(*function)();		/* function to call if recognized */
	int	func_parm;		/* parameter to function */
	char	*parm_ptr;		/* parameter ptr to function */
	int	parmno;
	int	offset;
	int	width;
	int	width_columns_wide_mode;
	int	decode_type;
	char	*string_ptr;		/* terminating chars for until type */
};
typedef struct ft_seq FT_SEQ;

FT_SEQ	*Seq_root[ MAX_PERSONALITY ];

#define DECIMAL_PARM		-2
#define BYTE_PARM		-3
#define STRING_PARM		-4
#define DELIM_STRING_PARM	-5
#define CHARACTER_PARM		-6
#define MODE_PARM		-7
#define LENGTH_PARM		-8
#define LENGTH_STRING_PARM	-9
#define FIXED_STRING_PARM	-10
#define UNTIL_STRING_PARM	-11
#define HP_STRING_PARM		-12
#define IBM_STRING_PARM		-13
#define IBM_DECIMAL_PARM	-14
#define HEX_PARM		-15
#define HEX_LENGTH_PARM		-16

unsigned char	Special_char[ MAX_PERSONALITY][ 256 ];

				/* 0x20  to  0x7E *//* 0xA0  to  0xFF */
#define C_PRINT		0
				/* head of sequence */
#define C_SPECIAL	1
				/* 0x01 to 0x1A 0x1C to 0x1F */
#define C_CONTROL	2
				/* 0x1B */
#define C_ESCAPE	3
				/* 0x7F */
#define C_DELETE	4
				/* 0x80  to 0x9F */
#define C_8BIT_SPECIAL	5

#define C_NULL_CHAR	6

#define DECODE_TYPE_MAX 2
int		Decode_type_no[ MAX_PERSONALITY ] = { 0 };
unsigned char	Decode_type_middle[ MAX_PERSONALITY ][ DECODE_TYPE_MAX ][ 256 ];

/**************************************************************************
* fct_decode_init
*	Initialize the decoder tree.
*	Allocate buffers for all windows.
*	Generate an array for determining the type of an incoming character.
**************************************************************************/
fct_decode_init()
{
        int     i;
        int     j;
	FT_SEQ *get_free_seq();
	int	personality;
	long	*malloc_run();

        for ( i = 0; i < Wins; i++ )
	{
		Decode[ i ] = ( FT_DECODE * ) malloc_run( sizeof( FT_DECODE ),
						"decode_information" );
		if ( Decode[ i ] == NULL )
		{
			printf( "ERROR: Decode malloc failed\n" );
			wait_return_pressed();
			exit( 1 );
		}
		Decode[ i ]->buffno = 0;
		Decode[ i ]->decode_type = 0;
	}
	for ( personality = 0; personality < MAX_PERSONALITY; personality++ )
	{
		for ( i = 0; i < 256; i++ )
			Special_char[ personality ][ i ] = C_PRINT;
		for ( i = 0x01; i <= 0x1F; i++ )
			Special_char[ personality ][ i ] = C_CONTROL;
		Special_char[ personality ][ 0x1B ] = C_ESCAPE;
		Special_char[ personality ][ 0x7F ] = C_DELETE;
		for ( i = 0x80; i <= 0x9F; i++ )
			Special_char[ personality ][ i ] = C_8BIT_SPECIAL;
		Special_char[ personality ][ 0 ] = C_NULL_CHAR;

		for ( j = 0; j < DECODE_TYPE_MAX; j++ )
			for ( i = 0; i < 256; i++ )
				Decode_type_middle[ personality ][ j ][ i ] = 0;

		Seq_root[ personality ] = get_free_seq();
		if ( Seq_root[ personality ] == NULL )
		{
			wait_return_pressed();
			exit( 1 );
		}
	}
}
/**************************************************************************
* get_free_seq
*	Allocate, initialize,  and return a node for the decoder tree.
**************************************************************************/
FT_SEQ *
get_free_seq()
{
	FT_SEQ		*pseq;
	long		*malloc_run();

	pseq = ( FT_SEQ * ) malloc_run( sizeof( FT_SEQ ), "sequence_node" );
	if ( pseq == NULL )
	{
		printf( "ERROR: Seq malloc failed\n" );
		return( NULL );
	}
	pseq->code = 0;
	pseq->decode_type = 0;
	pseq->next = NULL;
	pseq->also = NULL;
	pseq->function = NULL;
	pseq->func_parm = 0;
	pseq->parm_ptr = (char *) 0;
	pseq->parmno = 0;
	pseq->offset = 0;
	pseq->width = 0;
	pseq->width_columns_wide_mode = 0;
	pseq->string_ptr = (char *) 0;
	return( pseq );
}
/**************************************************************************
* dec_install
*	Install a sequence in the decoder tree.
**************************************************************************/
#include "options.h"
dec_install( name, string, function, func_parm, option, parm_ptr )
	char	*name;		/* char string for error messages */
	UNCHAR	*string;	/* termcap string to be installed */
	int	(*function)();	/* function to be called if recognized */
	int	func_parm;	/* parm to pass to above function */
	int	option;		/* 0 = normal 1 = contains % strings */
				/* 2 = next byte after string is a parm */
	char	*parm_ptr;	/* pointer to pass to above function */
{
	FT_SEQ	*pseq;
	UNCHAR	c;
	UNCHAR	*pstring;
	FT_SEQ	*install_seq();
	int	first;
	int	parm_cursor_incr;	/* row & col have had 1 added to them*/
	int	parmno;
	int	offset;
	int	value;
	int	width;
	int	width_columns_wide_mode;
	int	personality;
	int	first_personality;
	int	last_personality;
	int	until_len;
	int	i;
	long	*mymalloc();
	char	*until_string;
	char	*until_ptr;
	int	final_parm;

	if ( Opt_decode == 0 )		/* if set turn off decoder for testing*/
		return;	
	if ( string[ 0 ] == '\0' )	/* do not install null strings */
		return;		

	/******************************************************************
	* HP terminals have hp and ansi personalites - all other terminals
	* have one and can ignore this.
	******************************************************************/
	first_personality = X_pe;
	last_personality = X_pe;
	if ( Install_all_personalities )
	{
		first_personality = 0;
	}
	for (	personality = first_personality; 
		personality <= last_personality; 
		personality++ )
	{
	    pseq = Seq_root[ personality ]; /* start at root of decode tree */

	    pstring = string;
	    parmno = -1;
	    offset = 0;
	    value = 0;
	    parm_cursor_incr = 0;
    
	    first = 1;			/* flag for first construct - error */
	    while ( ( c = *pstring++ ) != '\0' )
	    {
		if ( (c == '%') && (option == 1) )
		{
			c = *pstring++;
			switch( c )
			{
			case '%':
				/******************************************
				* %% is one %
				******************************************/
				if ( first )
				{
					Special_char[ X_pe ][ '%' ] = C_SPECIAL;
					first = 0;
				}
				pseq = install_seq( (int) '%', 0, 0, 0, 0,
						    (char *) 0, pseq );
				break;
			case 'i':	/* row & col have had 1 added to them*/
				/******************************************
				* %i  means row and col are 1 based.
				******************************************/
				parm_cursor_incr = 1;
				break;
			case 'p':
				/******************************************
				* %p1 - %p9 %pA - %pC
				* indicate the following decimal etc belongs
				* to parameter 1 to 12 repectively
				******************************************/
				c = *pstring++;
				if ( c >= '1' && c <= '9' )
					parmno = c - '1';
				else if ( c >= 'A' && c <='C' )
					parmno = 9 + (c - 'A');
				else
					printf("bad %%p - '%c'\n", c  );
				break;
			case '\'':
				/******************************************
				* %'X' where X is a character is just the
				* character X. E.G %' ' is a space.
				******************************************/
				c = *pstring++;
				value = (int) c;
				c = *pstring++;
				if ( c != '\'' )
					printf("missing ' - '%c'\n", c  );
				break;
			case '{':		/* } makes showmatch happy */
				/******************************************
				* %{nnn} where nnn is one or more decimal
				* digits represents a byte with the value 
				* nnn.
				******************************************/
				while ( 1 )
				{
					c = *pstring++;
					if ( c == '\0' )
					{
						printf("{ without }\n" );
						break;
					}	
						/* { makes showmatch happy */
					if ( c == '}' )
						break;
					if ( c >= '0' && c <='9' )
					{
						value *= 10;
						value += ( c - '0' );
					}
					else
					{
						printf("bad char in {} '%c'\n",
							c  );
						break;
					}
				}
				break;
			case '+':
				/******************************************
				* %p1%' '%+%c
				* parameter has had an offset added to it.
				******************************************/
				offset = -value;
				break;
			case '-':
				/******************************************
				* parameter has had offset subtracted from it.
				******************************************/
				offset = value;
				break;
			case 'c':
				/******************************************
				* %p1%c Parameter is a single byte.
				******************************************/
				if ( first )
					printf( "leading %%c\n" );
				if ( parmno == -1 )
				{
					printf("%%c with no %%p\n" );
					break;
				}
				if ( parm_cursor_incr )
					offset -= 1;
				pseq = install_seq( BYTE_PARM,
					parmno, offset, 0, 0,
					(char *) 0, pseq );
				parmno = -1;
				offset = 0;
				value = 0;
				break;
			case 'd': case 's': case'l': case 'f': case 'S':
			case 'X': case 'h':
			case 'u': case 'H':
			case 'I':
			case 'b': case 'B':
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				/******************************************
				* Parameter has an optional width.
				******************************************/
				width = 0;
				while ( c >= '0' && c <= '9' )
				{
					width *= 10;
					width += ( c - '0' );
					c = *pstring++;
				}
				width_columns_wide_mode = width;
				/******************************************
				* Parameter has an optional width wide mode.
				******************************************/
				if ( c == '-' )
				{
					c = *pstring++;
					width_columns_wide_mode = 0;
					while ( c >= '0' && c <= '9' )
					{
					 width_columns_wide_mode *= 10;
					 width_columns_wide_mode += ( c - '0' );
					 c = *pstring++;
					}
				}
				if ( c == 'd' )
				{
					/**********************************
					* %d - number in decimal digits
					*********************************/
					if ( first )
						printf( "leading %%d\n" );
					if ( parmno == -1 )
					{
						printf("%%d with no %%p\n" );
						break;
					}
					if ( parm_cursor_incr )
						offset -= 1;
					pseq = install_seq( DECIMAL_PARM, 
						parmno, offset, width, 
						width_columns_wide_mode,
						(char *) 0, pseq );
					parmno = -1;
					offset = 0;
					value = 0;
					break;
				}
				else if ( c == 'X' )
				{
					/**********************************
					* %d - number in hex digits
					*********************************/
					if ( first )
						printf( "leading %%X\n" );
					if ( parmno == -1 )
					{
						printf("%%X with no %%p\n" );
						break;
					}
					if ( parm_cursor_incr )
						offset -= 1;
					pseq = install_seq( HEX_PARM, 
						parmno, offset, width, 
						width_columns_wide_mode,
						(char *) 0, pseq );
					parmno = -1;
					offset = 0;
					value = 0;
					break;
				}
				else if ( c == 's' )
				{
					/**********************************
					* %sN where N is the parameter number
					* 1-9 A-C
					* String of characters terminated
					* by character that follows
					*********************************/
					if ( first )
						printf( "leading %%s\n" );
					if ( parmno != -1 )
					{
						printf(
						  "do not use %%p with %%s\n" );
						break;
					}
					c = *pstring++;
					if ( (c >= '1') && (c <= '9') )
						parmno = c - '1';
					else if ( (c >= 'A') && (c <= 'C') )
						parmno = 9 + (c - 'A');
					else
					{
						printf( "%%s without parm #\n");
						break;
					}
							/* char after string */
					c = *pstring++;
					if ( c == '\0' )
					{
						printf(
						    "%%s at end of string\n" );
						break;
					}
					offset = (int) c;
					pseq = install_seq( STRING_PARM,
						parmno, offset, width, 
						width_columns_wide_mode,
						(char *) 0, pseq );
					parmno = -1;
					offset = 0;
					value = 0;
					break;
				}
				else if ( c == 'u' )
				{
					/**********************************
					* %uNLsss
					* where N is the parameter number
					*	1-9 A-C
					* where L is the length of the
					*	terminating string.
					* where sss is the terminating string.
					*********************************/
					if ( first )
						printf( "leading %%u\n" );
					if ( parmno != -1 )
					{
						printf(
						  "do not use %%p with %%u\n" );
						break;
					}
					c = *pstring++;
					if ( (c >= '1') && (c <= '9') )
						parmno = c - '1';
					else if ( (c >= 'A') && (c <= 'C') )
						parmno = 9 + (c - 'A');
					else
					{
						printf( "%%u without parm #\n");
						break;
					}
					c = *pstring++;
					if ( (c >= '1') && (c <= '9') )
						until_len = c - '0';
					else
					{
						printf( "%%u without len #\n");
						break;
					}
					until_string = (char *)
							mymalloc( until_len + 1,
							   "until string" );
					until_ptr = until_string;
					for ( i = 0; i < until_len; i++ )
					{
						c = *pstring++;
						if ( c == '\0' )
						{
							printf(
						    "%%u at end of string\n" );
							break;
						}
						*until_ptr++ = c;
					}
					if ( c == '\0' )
						break;
					*until_ptr = '\0';
					offset = until_len;
					pseq = install_seq( UNTIL_STRING_PARM,
						parmno, offset, width, 
						width_columns_wide_mode,
						until_string, pseq );
					parmno = -1;
					offset = 0;
					value = 0;
					break;
				}
				else if ( c == 'H' )
				{
					/**********************************
					* %HN
					* where N is the parameter number
					*	1-9 A-C
					* HP string parm - string ends with
					* 	a capital letter.
					*********************************/
					if ( first )
						printf( "leading %%H\n" );
					if ( parmno != -1 )
					{
						printf(
						  "do not use %%p with %%H\n" );
						break;
					}
					c = *pstring++;
					if ( (c >= '1') && (c <= '9') )
						parmno = c - '1';
					else if ( (c >= 'A') && (c <= 'C') )
						parmno = 9 + (c - 'A');
					else
					{
						printf( "%%H without parm #\n");
						break;
					}
					offset = (int) 0;
					pseq = install_seq( HP_STRING_PARM,
						parmno, offset, width, 
						width_columns_wide_mode,
						(char *) 0, pseq );
					parmno = -1;
					offset = 0;
					value = 0;
					break;
				}
				else if ( c == 'I' )
				{
					/**********************************
					* %IN
					* where N is the parameter number
					*	1-9 A-C
					* IBM_STRING_PARM - 
					* bytes with 0x40 clear and 0x20 set
					* ends  with 0x40 set   and 0x20 clear.
					*********************************/
					if ( first )
						printf( "leading %%I\n" );
					if ( parmno != -1 )
					{
						printf(
						  "do not use %%p with %%I\n" );
						break;
					}
					c = *pstring++;
					if ( (c >= '1') && (c <= '9') )
						parmno = c - '1';
					else if ( (c >= 'A') && (c <= 'C') )
						parmno = 9 + (c - 'A');
					else
					{
						printf( "%%I without parm #\n");
						break;
					}
					offset = (int) 0;
					pseq = install_seq( IBM_STRING_PARM,
						parmno, offset, width, 
						width_columns_wide_mode,
						(char *) 0, pseq );
					parmno = -1;
					offset = 0;
					value = 0;
					break;
				}
				else if ( ( c == 'b' ) || ( c == 'B' ) )
				{
					/**********************************
					* %bN
					* where N is the parameter number
					*	1-9 A-C
					* IBM_DECIMAL_PARM - 
					* bytes with 0x40 clear and 0x20 set
					* ends  with 0x40 set   and 0x20 clear.
					* put into parm(s) 2 bytes each
					*********************************/
					final_parm = 0;
					if ( c == 'B' )
						final_parm = 1;
					if ( first )
						printf( "leading %%b\n" );
					if ( parmno != -1 )
					{
						printf(
						  "do not use %%p with %%b\n" );
						break;
					}
					c = *pstring++;
					if ( (c >= '1') && (c <= '9') )
						parmno = c - '1';
					else if ( (c >= 'A') && (c <= 'C') )
						parmno = 9 + (c - 'A');
					else
					{
						printf( "%%b without parm #\n");
						break;
					}
					offset = (int) final_parm;
					pseq = install_seq( IBM_DECIMAL_PARM,
						parmno, offset, width, 
						width_columns_wide_mode,
						(char *) 0, pseq );
					parmno = -1;
					offset = 0;
					value = 0;
					break;
				}
				else if ( c == 'f' )
				{
					/**********************************
					* %fN
					* where N is the parameter number
					*	1-9 A-C
					* Fixed number of characters.
					*********************************/
					if ( first )
						printf( "leading %%f\n" );
					if ( parmno != -1 )
					{
						printf(
						  "do not use %%p with %%f\n" );
						break;
					}
					c = *pstring++;
					if ( (c >= '1') && (c <= '9') )
						parmno = c - '1';
					else if ( (c >= 'A') && (c <= 'C') )
						parmno = 9 + (c - 'A');
					else
					{
						printf( "%%f without parm #\n");
						break;
					}
					offset = 0;
					pseq = install_seq( FIXED_STRING_PARM,
						parmno, offset, width, 
						width_columns_wide_mode,
						(char *) 0, pseq );
					parmno = -1;
					offset = 0;
					value = 0;
					break;
				}
				else if ( c == 'l' )
				{
					/**********************************
					* %lN
					* where N is the parameter number
					*	1-9 A-C
					* Decimal number indicating the length
					*	of the matching %LN string later
					*	in the sequence.
					*********************************/
					if ( first )
						printf( "leading %%l\n" );
					if ( parmno != -1 )
					{
						printf(
						  "do not use %%p with %%l\n" );
						break;
					}
					c = *pstring++;
					if ( (c >= '1') && (c <= '9') )
						parmno = c - '1';
					else if ( (c >= 'A') && (c <= 'C') )
						parmno = 9 + (c - 'A');
					else
					{
						printf( "%%l without parm #\n");
						break;
					}
					offset = 0;
					pseq = install_seq( LENGTH_PARM, 
						parmno, offset, width, 
						width_columns_wide_mode,
						(char *) 0, pseq );
					parmno = -1;
					offset = 0;
					value = 0;
					break;
				}
				else if ( c == 'h' )
				{
					/**********************************
					* %hN
					* where N is the parameter number
					*	1-9 A-C
					* Hex number indicating the length
					*	of the matching %LN string later
					*	in the sequence.
					*********************************/
					if ( first )
						printf( "leading %%h\n" );
					if ( parmno != -1 )
					{
						printf(
						  "do not use %%p with %%h\n" );
						break;
					}
					c = *pstring++;
					if ( (c >= '1') && (c <= '9') )
						parmno = c - '1';
					else if ( (c >= 'A') && (c <= 'C') )
						parmno = 9 + (c - 'A');
					else
					{
						printf( "%%h without parm #\n");
						break;
					}
					offset = 0;
					pseq = install_seq( HEX_LENGTH_PARM, 
						parmno, offset, width, 
						width_columns_wide_mode,
						(char *) 0, pseq );
					parmno = -1;
					offset = 0;
					value = 0;
					break;
				}
				else if ( c == 'S' )
				{
					/**********************************
					* %SN
					* where N is the parameter number
					*	1-9 A-C
					* String where first character of
					* string is delimiter.  String ends
					* with another delimiter.
					*********************************/
					if ( first )
						printf( "leading %%S\n" );
					if ( parmno != -1 )
					{
						printf(
						  "do not use %%p with %%S\n" );
						break;
					}
					c = *pstring++;
					if ( (c < '1') || (c > '9') )
					{
						printf(
						  "%%S without parm #\n" );
						break;
					}
					parmno = c - '1';
					offset = -1;
					pseq = install_seq( DELIM_STRING_PARM,
						parmno, offset, width, 
						width_columns_wide_mode,
						(char *) 0, pseq );
					parmno = -1;
					offset = 0;
					value = 0;
					break;
				}
				else
				{
					/**********************************
					* in case statement but not in if then
					*********************************/
					printf("bad sequence - %% # '%c'\n", c);
					break;
				}
			case 'L':
				/******************************************
				* %LN
				* where N is the parameter number 1-9 A-C
				* Length of string is specified in matching
				* %lN earlier in string.
				******************************************/
				if ( first )
					printf( "leading %%L\n" );
				if ( parmno != -1 )
				{
					printf("do not use %%p with %%L\n" );
					break;
				}
				c = *pstring++;
				if ( (c >= '1') && (c <= '9') )
					parmno = c - '1';
				else if ( (c >= 'A') && (c <= 'C') )
					parmno = 9 + (c - 'A');
				else
				{
					printf( "%%L without parm #2\n" );
					break;
				}
				offset = -1;
				pseq = install_seq( LENGTH_STRING_PARM,
					parmno, offset, 0, 0,
					(char *) 0, pseq );
				parmno = -1;
				offset = 0;
				value = 0;
				break;
			case 'C':
				/******************************************
				* %CN
				* where N is the parameter number 1-9 A-C
				* Single character as a string.
				******************************************/
				if ( first )
					printf( "leading %%C\n" );
				if ( parmno != -1 )
				{
					printf("do not use %%p with %%S\n" );
					break;
				}
				c = *pstring++;
				if ( (c >= '1') && (c <= '9') )
					parmno = c - '1';
				else if ( (c >= 'A') && (c <= 'C') )
					parmno = 9 + (c - 'A');
				else
				{
					printf( "%%C without parm #\n" );
					break;
				}
							/* char */
				c = *pstring++;
				if ( c == '\0' )
				{
					printf( "%%C at end of string\n" );
					break;
				}
				offset = (int) c;
				pseq = install_seq( CHARACTER_PARM,
					parmno, offset, 0, 0,
					(char *) 0, pseq );
				parmno = -1;
				offset = 0;
				value = 0;
				break;
			case 'm':
				/******************************************
				* %mNX
				* where N is the parameter number 1-9 A-C
				* This is a decimal number optionally 
				* preceeded by the character X.
				* E.G. %m1? would match  10   and  ?10 .
				******************************************/
				width = 0;
				width_columns_wide_mode = 0;
				if ( first )
					printf( "leading %%m\n" );
				if ( parmno != -1 )
				{
					printf("do not use %%p with %%m\n" );
					break;
				}
				c = *pstring++;
				if ( (c >= '1') && (c <= '9') )
					parmno = c - '1';
				else if ( (c >= 'A') && (c <= 'C') )
					parmno = 9 + (c - 'A');
				else
				{
					printf( "%%C without parm #\n" );
					break;
				}
							/* char */
				c = *pstring++;
				if ( c == '\0' )
				{
					printf( "%%C at end of string\n" );
					break;
				}
				offset = (int) c;
				pseq = install_seq( MODE_PARM, 
					parmno, offset, width,
					width_columns_wide_mode,
					(char *) 0, pseq );
				parmno = -1;
				offset = 0;
				value = 0;
				break;
			default:
				printf( "unknown %% '%c' 0x%x\n", c, (int) c );
				break;
			}
		}
		else if ( ( c == '$' ) && ( *pstring == '<' ) )
		{					/* ignore padding */
			/**************************************************
			* $<20>   $<20/>   $<20*>
			* padding - mandatory padding - proportional padding
			**************************************************/
			c = *pstring++;
			if ( c == '<' )
			{
				while( 1 )
				{
					c = *pstring++;
					if ( c == '\0' )
					{
						printf( "$< without >\n" );
						break;
					}
					else if ( c == '>' )
						break;
				}
			}
			else
				printf( "$ without <\n" );
		}
		else if ( c == 0x80 )
		{
			/**************************************************
			* Embedded null - ick ick - now we cannot handle 0x80.
			**************************************************/
			if ( first )
				printf( "leading octal 200\n" );
			pseq = install_seq( 0, 0, 0, 0, 0, (char *) 0, pseq );
		}
		else
		{
			/**************************************************
			* not a %X, padding, or embedded null.
			**************************************************/
			if ( first )
			{
				Special_char[ X_pe ][ c ] = C_SPECIAL;
				first = 0;
			}
			pseq = install_seq( (int) c, 0, 0, 0, 0,
					    (char *) 0, pseq );
		}
		if ( pseq == NULL )
		{
			printf( "    Will be used for output only: %s\n", 
				name );
			return;
		}
	    }
	    if ( option == BYTE_PARM_FOLLOWS_OPTION )
	    {
		pseq = install_seq( BYTE_PARM, 0, 0, 0, 0, (char *) 0, pseq );
	    }
	    if ( pseq == NULL )
	    {
		printf( "    Will be used for output only:: %s\n", name );
		return;
	    }
	    if ( pseq->next != NULL )
	    {
		printf( "duplicate to longer sequence\n" );
		printf( "    Will be used for output only:: %s\n", name );
		return;
	    }
	    pseq->function = function;
	    pseq->func_parm = func_parm;
	    pseq->parm_ptr = parm_ptr;
	}
}
/**************************************************************************
* install_seq
*	The next part of a sequence being added has the code "code", which
*	should be returned as parameter number "parmno" and will have had
*	the offset "offset" added to it, and is maximum width "width".
*	"string_ptr" is an optional string for UNTIL strings.
*	Starting at node "pseq", determine of we are still tracking a
*	sequence that is already here - return the new position -
*	or are starting a new branch - add and return the new position.
*	Return pointer to new node.
*	Return NULL if cannot add to the tree or this sequence is already
*	in the tree and ends here. 
**************************************************************************/
FT_SEQ *
install_seq( code, parmno, offset, width, width_columns_wide_mode,
							string_ptr, pseq )
	int	code;
	int	parmno;
	int	offset;
	int	width;
	int	width_columns_wide_mode;
	char	*string_ptr;
	FT_SEQ	*pseq;
{
	FT_SEQ	*p;
	FT_SEQ	*new;
	FT_SEQ	*prev;
	int	prev_is_thru_next;

	p = pseq->next;
	if ( p == NULL )
	{
		if ( (new = get_free_seq()) == NULL )
			return( NULL );
		pseq->next = new;
		new->code = code;
		new->decode_type = 0;	/* ??? */
		new->parmno = parmno;
		new->offset = offset;
		new->width = width;
		new->width_columns_wide_mode = width_columns_wide_mode;
		new->string_ptr = string_ptr;
		return( new );
	}
	prev = pseq;
	prev_is_thru_next = 1;
	while( 1 )
	{
		if (  p->code == code     && p->parmno == parmno 
		   && p->offset == offset && p->width == width
		   && p->width_columns_wide_mode == width_columns_wide_mode )
		{
		   if (  ( p->code != UNTIL_STRING_PARM )
		      || ( strcmp( p->string_ptr, string_ptr ) == 0 )
		      )
		   {
			if ( p->function != NULL )
			{
				printf( "duplicate sequence\n" );
				/*
				printf( 
				"   code=0x%x parmno=%d offset=%d width=%d\n",
					code, parmno, offset, width );
				*/
				return( NULL );
			}
			return( p );
		    }
		}
		if ( ( code >= 0 ) && ( p->code < 0 ) )
		{
			if ( (new = get_free_seq()) == NULL )
				return( NULL );
			if ( prev_is_thru_next )
				prev->next = new;
			else
				prev->also = new;
			new->also = p;
			new->code = code;
			new->decode_type = 0;	/* ??? */
			new->parmno = parmno;
			new->offset = offset;
			new->width = width;
			new->width_columns_wide_mode = width_columns_wide_mode;
			new->string_ptr = string_ptr;
			return( new );
		}
		if ( p->also == NULL )
		{
			if ( (new = get_free_seq()) == NULL )
				return( NULL );
			p->also = new;
			new->code = code;
			new->decode_type = 0;	/* ??? */
			new->parmno = parmno;
			new->offset = offset;
			new->width = width;
			new->width_columns_wide_mode = width_columns_wide_mode;
			new->string_ptr = string_ptr;
			return( new );
		}
		prev = p;
		prev_is_thru_next = 0;
		p = p->also;
	}
}
/**************************************************************************
* install_decode_type
*	Starting at the node "pseq" ( actually the root of the tree )
*	locate the node where "string" would decode to.
*	Mark the node as the base of the decode type sequence number
*	"decode_type".
*	These are used to indicate that a sequence that starts like this
*	is not finished until you encounter a character other than
*	the characters shown in install_decode_type_middle ( below ).
*	Ansi type terminals have a lot of \E[ followed by numbers, semicolons,
*	question marks, etc which gets partial matches to a large section
*	of the decoder tree.
*	This decode-type makes it give up as a partial match without burning
*	up the cpu finding out that we have 500 partial matches.
**************************************************************************/
install_decode_type( decode_type, string, pseq )
	int		decode_type;
	unsigned char	*string;
	FT_SEQ		*pseq;
{
	FT_SEQ		*p;
	unsigned char	*s;
	unsigned char	c;

	p = pseq->next;
	if ( p == NULL )
	{
		printf( "ansi install failed start\n" );
		return( -1 );
	}
	s = string;
	c = *s++;
	while( 1 )
	{
		if (  p->code == c   && p->parmno == 0 
		   && p->offset == 0 && p->width == 0
		   && p->width_columns_wide_mode == 0 ) /*???*/
		{
			c = *s++;
			if ( c == '\0' )
			{
				p->decode_type = decode_type;
				return( 0 );
			}
			p = p->next;
			if ( p == NULL )
			{
				printf( "ansi install failed next\n" );
				return( -1 );
			}
		}
		else
		{
			p = p->also;
			if ( p == NULL )
			{
				printf( "ansi install failed also\n" );
				return( -1 );
			}
		}
	}
}
/**************************************************************************
* install_decode_type_middle
*	Put the characters in the string "string" in the character arrays
*	for decode type number "decode_type" and mark them with "setting" (1).
*	See install_decode_type above.
**************************************************************************/
install_decode_type_middle( decode_type, string, setting )
	int		decode_type;
	unsigned char	*string;
	int		setting;
{
	unsigned char	*s;
	unsigned char	c;
	int		i;

	s = string;
	if ( *s == ' ' )
	{
		/**********************************************************
		*  A leading space means all printable characters.
		**********************************************************/
		for ( i = ' '; i <= '~'; i++ )
		    Decode_type_middle[ X_pe ][ decode_type ][ i ] = setting;
		s++;
	}
	while( ( c = *s++ ) != '\0' )
	{
		Decode_type_middle[ X_pe ] [ decode_type ][ c ] = setting;
	}
}
/**************************************************************************
* fct_monitor
*	"count" number of characters pointed to by "ptr" have been output
*	on window number "window" which is in monitor mode.
*	Output them so that they are visible and do not cause errors.
**************************************************************************/
fct_monitor( window, ptr, count )
	int	window;
	char	*ptr;
	int	count;
{
	int		i;
	unsigned char	*p;
	unsigned char	c;

	if ( window >= Wins )
		return;
	fct_set_outwinno( window );
	p = (unsigned char *) ptr;
	for ( i = 0; i < count; i++ )
	{
		c = *p++;
						/* escape backslash and ^
						   so that they are 
						   distinguishable from 
						   artifically generated ones.
						*/
		if ( c == '\\' )
			fct_char_logged( '\\' );
		else if ( c == '^' )
			fct_char_logged( '\\' );
		fct_char_logged( c );
	}
	fct_done_outwinno();
}
/**************************************************************************
* fct_transparent
*	"count" number of characters pointed to by "ptr" have been output
*	on window number "window" which is in transparent mode.
*	Send them to the terminal.
**************************************************************************/
fct_transparent( winno, ptr, count )
	int	winno;
	char	*ptr;
	int	count;
{
	if ( winno >= Wins )
		return;
	if ( winno_is_curwinno( winno ) )
	{
		term_write( ptr, count );
	}
}
#include "print.h"
char	*T_transparent_print_hold_if_partial = { (char *) 0 };
int	Len_transparent_print_hold_if_partial = 0;
#include "tpnotify.h"
/**************************************************************************
* fct_printer
*	"count" number of characters pointed to by "ptr" have been output
*	on window number "window" which is in 'printer' mode.
*	Send them to the printer.
**************************************************************************/
#define SENDBUFFMAX 1000
fct_printer( winno, ptr, count )
	int	winno;
	char	*ptr;
	int	count;
{
	int	seconds_delayed;

	seconds_delayed = 0;
	if ( winno >= Wins )
		return;
	if ( Hp_transparent_print_reply )
		transparent_print_notify( ptr, count, winno );
	else
	{
		long	start_time;
		long	time();

		char	sendbuff[ SENDBUFFMAX + 1 ];
		int	sendcount;

		sendcount = check_transparent_print_hold(
				winno, ptr, count, sendbuff, SENDBUFFMAX );
		if ( sendcount <= 0 )
			return;
		start_time = time( (long *) 0 );
		term_out_transparent_print( sendbuff, sendcount );
		seconds_delayed = time( (long *) 0 ) - start_time;
	}
	set_print_active( winno, seconds_delayed );
}
#define	MAX_HOLD_CHARS 20
char	Printer_hold_buffer[ TOPWIN ][ MAX_HOLD_CHARS + 1 ] = { '\0' };
int	Printer_hold_buffer_count[ TOPWIN ] = { 0 };
clear_transparent_print_hold( winno )
	int	winno;
{
	Printer_hold_buffer_count[ winno ] = 0;
}
/*************************************************************************
* check_transparent_print_hold
*	Go across buffer looking for hold if partial ( which may hide
*	transparent print off ) or transparent print off.
*	Do not send "hold if partial" until we have character after it.
*	If match transparent print off - discard it.
*	If hit end of string in middle of one of these, hold for
*	more characters.
*************************************************************************/
check_transparent_print_hold( winno, ptr, count, sendbuff, sendbuffmax )
	int	winno;
	char	*ptr;
	int	count;
	char	*sendbuff;
	int	sendbuffmax;
{
	int	sendcount;
	int	newcount;
	int	holdlen;
	int	hc;
	int	offlen;
	int	oc;
	int	out;
	int	i;
	int	j;

	for ( newcount = 0;
	      newcount < Printer_hold_buffer_count[ winno ];
	      newcount++ )
	{
		if ( newcount >= sendbuffmax )
			break;
		sendbuff[ newcount ] = Printer_hold_buffer[ winno ][ newcount ];
			
	}
	for ( j = 0; j < count; j++, newcount++ )
	{
		if ( newcount >= sendbuffmax )
			break;
		sendbuff[ newcount ] = ptr[ j ];
	}
	offlen = Len_transparent_print_off;
	holdlen = Len_transparent_print_hold_if_partial;
	sendcount = 0;		/* send  0 to < sendcount */
				/* keep sendcount to < newcount */
	out = 0;		/* points to current char under consideration */
	while( out < newcount )
	{
		/*************************************************************
		* See if we have a match with the hold string.
		*************************************************************/
		hc = 0;
		while(  ( hc < holdlen )
		     && ( out < newcount )
		     && ( sendbuff[ out ] ==
			  T_transparent_print_hold_if_partial[ hc ] )
		     )
		{		/* matching or matched hold */
			hc++;
			out++;
		}
		if ( hc > 0 )
		{	
			/***************************************************
			* Matched at least one character.
			* Want to send a partial match plus one character or
			* a full match plus one character to the printer.
			* If out of characters, it doesn't matter - hold them.
			*****************************************************/
			if ( out >= newcount )
			{	/* matching or matched hold & out of chars */
				break;
			}
			/***************************************************
			* Otherwise we have a full or partial match and 
			* another character, so they are ready to go.
			*****************************************************/
			out++;
			sendcount = out;
			continue;
		}
		/*************************************************************
		*************************************************************/
		oc = 0;
		while(  ( oc < offlen )
		     && ( out < newcount )
		     && ( sendbuff[ out ] == Raw_transparent_print_off[ oc ] )
		     )
		{		/* matched or matching */
			oc++;
			out++;
		}
		if ( oc > 0 )
		{
			/*****************************************************
			* If matched transparent print off - discard.
			*****************************************************/
			if ( oc >= offlen )
			{
				for ( i = out; i < newcount; i++ )
					sendbuff[ i - offlen ] = sendbuff[ i ];
				out -= offlen;
				newcount -= offlen;
				sendcount = out;
				continue;
			}
			/*****************************************************
			* If matching when out of chars - hold them.
			*****************************************************/
			if ( out >= newcount )
			{
				break;
			}
			/*****************************************************
			* Partial match broken by char at out - send.
			*****************************************************/
			out++;
			sendcount = out;
			continue;
		}
		/*************************************************************
		* No match - send.
		*************************************************************/
		out++;
		sendcount = out;
	}
	for ( i = sendcount; i < newcount; i++ )
		Printer_hold_buffer[ winno ][ i ] = sendbuff[ i ];
	Printer_hold_buffer_count[ winno ] = newcount - sendcount;
	return( sendcount );
}
/**************************************************************************
* fct_decode_string
*	A null terminated string of characters pointed to by "ptr" have
*	been output on window number "window" which is in 'normal' mode.
**************************************************************************/
fct_decode_string( window, ptr )
	int	window;
	char	*ptr;
{
	int	count;

	count = strlen( ptr );
	fct_decode( window, (UNCHAR *) ptr, count );
}

#include "hpattr.h"
#include "myattrs.h"
int F_transparent_print_8_bit = 0; /* ??? */
int F_graph_screen_8_bit = 0; /* ??? */
int F_special_8_bit = 0; /* ??? */
/**************************************************************************
* fct_decode
*	"count" number of characters pointed to by "ptr" have been output
*	on window number "window" which is in 'normal'mode.
*	Try to figure out what they mean using the decoder tree and
*	update the window buffer and terminal appropriately.
**************************************************************************/
fct_decode( window, ptr, count )
	int	window;
	UNCHAR	*ptr;
	int	count;
{
	REG	FT_WIN		*outwin;	
	REG	unsigned char	*p_col_changed;
	REG	FTCHAR		*p_ftchars;
	REG	int		c;
	REG	int		wrap_margin_right;
	REG	FTCHAR		ftattrs;
	REG	int		mask;
	REG	int		cc;
	REG	FT_DECODE	*outdecode;
		int		personality;

	if ( window >= Wins )
		return;
	fct_set_outwinno( window );
	Outdecode = Decode[ window ];
	outdecode = Outdecode;
	outwin = Outwin;
	while( count > 0 )
	{
	    personality = outwin->personality;
	    c = *ptr++;
	    c &= 0x00FF;
	    count--;
	    if ( outwin->graph_screen_on )
	    {					/* graphscreen on */
		/**********************************************************
		* Watch for (only) graph screen off.
		**********************************************************/
		if ( outwin->control_8_bit_on )
		{
			if ( (c >= 0x80) && (c <= 0x9F) )
			{
				graph_screen_char( 0x1B );
				c = '@' + (c & 0x1F);
			}
		}
		else if (  ( F_graphics_8_bit == 0 )
		        && ( outwin->graphics_8_bit == 0 )
			&& ( F_graph_screen_8_bit == 0 ) )
		{
			c &= 0x7F;
		}
		graph_screen_char( (UNCHAR) c );
	    }
	    else if ( outwin->special_function != NULL )
	    {					/* special collection */
		/**********************************************************
		* Watch for (only) special collection off.
		**********************************************************/
		if ( outwin->control_8_bit_on )
		{
			if ( (c >= 0x80) && (c <= 0x9F) )
			{
				(*(outwin->special_function))(
						outwin->special_ptr,
						0x1B );
				c = '@' + (c & 0x1F);
			}
		}
		else if (  ( F_graphics_8_bit == 0 )
		        && ( outwin->graphics_8_bit == 0 )
			&& ( F_special_8_bit == 0 ) )
		{
			c &= 0x7F;
		}
		(*(outwin->special_function))( outwin->special_ptr, (UNCHAR) c);
	    }
	    else if ( outwin->transparent_print_on  == 0 )
	    {
		if ( outdecode->buffno == 0 )
		{
			mask = 0xFF;
			if (  ( outwin->control_8_bit_on == 0 )
			   && ( outwin->graphics_8_bit == 0 )
			   && ( F_graphics_8_bit == 0 ) )
			{
				c &= 0x007F;
				mask = 0x007F;
			}
			switch ( Special_char[ personality ][ c ] )
			{
			case C_PRINT:	/* 0x20  to  0x7E *//* 0xA0  to  0xFF */
				if (  ( outwin->onscreen == 0 )
				   || ( outwin->insert_mode )
				   || ( outwin->xenl )
				   || ( outwin->real_xenl ) )
				{
					fct_char( (UNCHAR) c );
					outdecode->lastchar = c;
					break;
				}
				/******************************************
				* Try to hot shot normal characters in the
				* middle of the screen on their way without
				* burning up too much cpu.  Go back to 
				* normal processing as soon as anything
				* complicated happens.
				******************************************/
				outwin->row_changed[ outwin->row ] = 1;
				ftattrs = outwin->ftattrs;
				wrap_margin_right = outwin->wrap_margin_right;
				p_col_changed = 
					&( outwin->col_changed[ outwin->col ] );
				p_ftchars = 
					&( outwin->ftchars[ outwin->row ]
							  [ outwin->col ] );
				while( 1 )
				{
					putchar( c );
					*p_col_changed++ = 1;
					if ( oF_hp_attribute == 0 )
					{
					  *p_ftchars++ = (FTCHAR) (c | ftattrs);
					}
					else
					{
					  *p_ftchars &= ( ~ FTCHAR_CHAR_MASK );
					  *p_ftchars++ |= 
						( c | ATTR_HP_CHAR_PRESENT );
					}
					if ( outwin->col < wrap_margin_right )
						outwin->col++;
					else if ( outwin->col ==
							wrap_margin_right )
					{
						outwin->col++;
						ft_past_right_margin();
						outdecode->lastchar = c;
						break;
					}
					else if ( outwin->col < 
							outwin->col_right_line )
					{
						outwin->col++;
					}
					else
					{
						outdecode->lastchar = c;
						break;
					}
					if ( count <= 0 )
					{
						outdecode->lastchar = c;
						break;
					}
					cc = *ptr;
					cc &= mask;
					if ( Special_char[ personality ][ cc ] 
						!= C_PRINT )
					{
						outdecode->lastchar = c;
						break;
					}
					c = cc;
					ptr++;
					count--;
				}
				break;
			case C_SPECIAL:	/* head of sequence */
				chk_char( (UNCHAR) c );
				break;
			case C_8BIT_SPECIAL:	/* 0x80  to 0x9F */
				if ( outwin->control_8_bit_on )
				{
					chk_char( 0x1B );
					c = '@' + (c & 0x1F);
					chk_char( (UNCHAR) c );
				}
				else
					fct_char( (UNCHAR) c );
				break;
			case C_DELETE:	/* 0x7F */
			case C_ESCAPE:	/* 0x1B */
			case C_CONTROL:	/* 0x01 to 0x1A 0x1C to 0x1F */
				fct_char( (UNCHAR) c );
				break;
			default:	/* 0x00 */
				break;
			}
		}
		else if ( outdecode->decode_type == 0 )
		{			/* in normal sequence */
			if ( outwin->control_8_bit_on )
			{
				if ( (c >= 0x80) && (c <= 0x9F) )
				{
					outdecode->decode_type = 0;
					chk_char( 0x1B );
					c = '@' + (c & 0x1F);
				}
			}
			else if (  ( F_graphics_8_bit == 0 )
				&& ( outwin->graphics_8_bit == 0 ) )
			{
				c &= 0x7F;
			}
			chk_char( (UNCHAR) c );
		}
		else
		{			/*  middle of a decode_type sequence */
			unsigned char	*middle;

			/**************************************************
			* Try not to furiously hunt for the meaning of a
			* sequence that is known by its form to be partial.
			**************************************************/
			middle = 
			 Decode_type_middle[ personality ]
					   [ outdecode->decode_type - 1 ];
			while( 1 )
			{
				if ( outwin->control_8_bit_on )
				{
					if ( (c >= 0x80) && (c <= 0x9F) )
					{
						outdecode->decode_type = 0;
						chk_char( 0x1B );
						c = '@' + (c & 0x1F);
						chk_char( (UNCHAR) c );
						break;
					}
				}
				else if (  ( F_graphics_8_bit == 0 )
					&& ( outwin->graphics_8_bit == 0 ) )
				{
					c &= 0x7F;
				}
				if (  ( middle[ c ] == 0 )
				   || ( outdecode->buffno >= ( MAX_BUFF - 1 ) ))
				{		/* not in middle or overflow */
					outdecode->decode_type = 0;
					chk_char( (UNCHAR) c );
					break;
				}
				outdecode->buff[ outdecode->buffno++ ] = c;	
				if ( count <= 0 )
					break;
				c = *ptr++;
				c &= 0x00FF;
				count--;
			}
		}
	    }
	    else
	    {					/* transparent print on */
		/**********************************************************
		* Watch for (only) transparent print off.
		**********************************************************/
		if ( outwin->control_8_bit_on )
		{
			if ( (c >= 0x80) && (c <= 0x9F) )
			{
				chk_transparent_print_char( 0x1B );
				c = '@' + (c & 0x1F);
			}
		}
		else if (  ( F_graphics_8_bit == 0 )
		        && ( outwin->graphics_8_bit == 0 )
			&& ( F_transparent_print_8_bit == 0 ) )
		{
			c &= 0x7F;
		}
		chk_transparent_print_char( (UNCHAR) c );
	    }
	}
	fct_done_outwinno();
}
/**************************************************************************
* chk_transparent_print_char
*	Watch for transparent print off sequence.
**************************************************************************/
int	F_allow_embedded_transparent_print_off = 0;
chk_transparent_print_char( c )
	UNCHAR	c;
{
	int	i;

	Outdecode->buff[ Outdecode->buffno++ ] = c;	
	while( 1 )
	{
		for ( i = 0; i < Outdecode->buffno; i++ )
		{
		   if ( Outdecode->buff[ i ] != Raw_transparent_print_off[ i ] )
			break;
		}
		if ( i >= Len_transparent_print_off )
		{					/* matched - done */
			fct_transparent_print_off();
			Outdecode->buffno = 0;
			Outdecode->decode_type = 0;
			return;
		}
		else if ( i >= Outdecode->buffno )
		{					/* matched - not done */
			return;
		}
		else
		{					/* no match */
			if ( Outdecode->buffno <= 
					Len_transparent_print_hold_if_partial )
			{
				for ( i = 0; i < Outdecode->buffno; i++ )
				{
				    if ( Outdecode->buff[ i ] !=
				     T_transparent_print_hold_if_partial[ i ] )
					break;
				}
				if ( i >= Outdecode->buffno )
				{			/* hold for more chars*/
					return;
				}
			}
			if ( F_allow_embedded_transparent_print_off == 0 )
			{
				for ( i = 0; i < Outdecode->buffno; i++ )
					term_putc( Outdecode->buff[ i ] );
				Outdecode->buffno = 0;
				Outdecode->decode_type = 0;
				return;
			}
			term_putc( Outdecode->buff[ 0 ] );
			Outdecode->buffno--;
			if ( Outdecode->buffno <= 0 )
			{				/* out of characters */
				Outdecode->buffno = 0;
				Outdecode->decode_type = 0;
				return;
			}
			for ( i = 0; i < Outdecode->buffno; i++ )
				Outdecode->buff[ i ] = Outdecode->buff[ i + 1 ];
		}
	}
}
start_substitute()
{
	Outdecode->buffno = 0;
	Outdecode->decode_type = 0;
}
end_substitute()
{
}
int Partial = 0;
/**************************************************************************
* chk_char
*	Add one more character to the sequence and see if it means anything
*	yet.
*	If it does call the appropriate DECODE module to handle the sequence.
*	If it is a partial match, return for another characeter.
*	If we have figured out that it is a sequence but we do not recognize
*	it then complain.
**************************************************************************/
chk_char( c )
	UNCHAR	c;
{
	REG	FT_DECODE	*outdecode;
		FT_SEQ		*pseq;
		int		i;
		FT_SEQ		*check_alsos();
		UNCHAR	string_store[ MAX_BUFF ];/* strings of cur sequence */

	outdecode = Outdecode;
	Partial = 0;
	outdecode->buff[ outdecode->buffno++ ] = c;	
	pseq = Seq_root[ Outwin->personality ]->next;
	if ( (outdecode->buffno < MAX_BUFF) && (pseq != NULL) )
	{
		pseq = check_alsos( pseq, 0,
				    outdecode->buff, outdecode->buffno,
				    string_store, 0 );
		if ( pseq != NULL )
		{
			(*(pseq->function))(
				pseq->func_parm,
				pseq->parm_ptr,
				Dec_parms_valid,
				Dec_parm,
				Dec_string_parm,
				Dec_string_parms_valid );
			outdecode->buffno = 0;
			outdecode->decode_type = 0;
			return;
		}
	}
	if ( Partial )
		return;
	else
	{
		if ( outdecode->buffno == 1 )
			outdecode->lastchar = outdecode->buff[ 1 ];
		if (  oF_graphics_escape_control
			&& ( outdecode->buffno == 2 )
			&& ( outdecode->buff[ 0 ] == '\033' )
			&& ( outdecode->buff[ 1 ] < ' ' ) )
		{
			fct_graphics_escape_control( outdecode->buff[ 1 ] );
			outdecode->buffno = 0;
			outdecode->decode_type = 0;
			return;
		}
		if (  oF_graphics_escape_delete
			&& ( outdecode->buffno == 2 )
			&& ( outdecode->buff[ 0 ] == '\033' )
			&& ( outdecode->buff[ 1 ] == '\177' ) )
		{
			fct_graphics_escape_control( outdecode->buff[ 1 ] );
			outdecode->buffno = 0;
			outdecode->decode_type = 0;
			return;
		}
		if ( Opt_error_ignore )
		{
		}
		else if ( Opt_error_pass )
		{
			term_write( (char *) outdecode->buff,
					     outdecode->buffno );
		}
		else
		{
			for ( i = 0; i < outdecode->buffno; i++ )
				fct_char_logged( outdecode->buff[ i ] );
			term_beep();
			term_outgo();
			error_record( outdecode->buff, outdecode->buffno );
			term_beep();
		}
		outdecode->buffno = 0;
		outdecode->decode_type = 0;
	}
}
/**************************************************************************
* check_alsos
*	Go across the tree at the same level as the node "pseq_in" looking
*	for a branch that fits the current character pointed to by "buff".
*	There are "buffno" characters in "buff".
*	Update "parms_valid" "string_store" and "string_parms_valid" as
*	matches are made.
**************************************************************************/
FT_SEQ *
check_alsos( pseq_in, parms_valid, buff, buffno, string_store,
							string_parms_valid )	
	FT_SEQ	*pseq_in;
	int	parms_valid;
	UNCHAR	*buff;
	int	buffno;
	UNCHAR	*string_store;
	int	string_parms_valid;
{
	REG	FT_SEQ	*pseq;
	REG	int	code;
	REG	int	c;
	REG	FT_SEQ	*found;
		FT_SEQ	*check_node();

	pseq = pseq_in;
	c = *buff;
	if ( pseq == NULL )
	{
		printf( "NULL pointer to check_alsos\r\n" );
		term_outgo();
		return( NULL );
	}
	while( 1 )
	{
		/**********************************************************
		* Quickly skip single character nodes that do not match.
		**********************************************************/
		while( 1 )
		{
			if ( pseq == NULL )
				return( NULL );
			code = pseq->code;
			if ( ( code < 0 ) || ( code == c ) )
				break;
			pseq = pseq->also;
		}
		/**********************************************************
		* See if we can match down this branch.  Success returns
		* a pointer to the action node.
		**********************************************************/
		found = check_node( pseq, parms_valid,
				    buff, buffno, string_store,
				    string_parms_valid );
		if ( found != NULL )
			return( found );
		/**********************************************************
		* Go on across.
		**********************************************************/
		pseq = pseq->also;
	}
}
/**************************************************************************
* check_node
*	Check down this branch with node "pseq_in" for a decode match.
*	On a final match, return the pointer to the action node.
*	On a match of this node, go down another level and try the next
*		character.
*	On a partial match but out of characeter, set Partial and return
*		for more characeters.
**************************************************************************/
FT_SEQ *
check_node( pseq_in, parms_valid, buff, buffno, string_store,
							string_parms_valid )
	FT_SEQ	*pseq_in;
	int	parms_valid;
	UNCHAR	*buff;
	int	buffno;
	UNCHAR	*string_store;
	int	string_parms_valid;
{
	REG	FT_SEQ	*pseq;
	REG	UNCHAR	*pb;		/* ptr to cur char in input stream */
	REG	int	parmval;
		int	i;
		int	delim;
		int	char_count;
		int	code;
		FT_SEQ	*check_alsos();

	pseq = pseq_in;
	if ( pseq == NULL )
	{
		printf( "NULL pointer to check_node\r\n" );
		term_outgo();
		return( NULL );
	}
	pb = buff;
	code = pseq->code;
	if ( code < 0 )
	{
	    int	parmno;
	    int	parms_valid_bit;

	    parmno = pseq->parmno;
	    parms_valid_bit = ( 1 << parmno );
	    switch ( code )
	    {
	    /**********************************************************
	    * Look for a string of decimal digits of max length "width"
	    **********************************************************/
	    case DECIMAL_PARM:
		parms_valid |= parms_valid_bit;
		parmval = 0;
		if ( *pb >= '0' && *pb <= '9' )
		{
			if ( pseq->width == 0 )
			{
				do
				{
					parmval *= 10;
					parmval += ( *pb - '0');
					pb++;
					buffno--;
					if ( buffno <= 0 )
					{
						Partial = 1;
						return( NULL );
					}
				} while ( *pb >= '0' && *pb <= '9' );
			}
			else
			{
				parmval *= 10;
				parmval += ( *pb - '0');
				pb++;
				buffno--;
				for ( i = 1; i < pseq->width; i++ )
				{
					if ( buffno <= 0 )
					{
						Partial = 1;
						return( NULL );
					}
					if ( *pb < '0' || *pb > '9' )
						return( NULL );
					parmval *= 10;
					parmval += ( *pb - '0');
					pb++;
					buffno--;
				} 
			}
			Dec_parm[ parmno ] =
				parmval + pseq->offset;
		}
		else
			return( NULL );
		break;

	    /**********************************************************
	    * Look for a string of hex digits of max length "width"
	    **********************************************************/
	    case HEX_PARM:
		parms_valid |= parms_valid_bit;
		parmval = 0;
		if ( (*pb >= '0' && *pb <= '9') || (*pb >= 'A' && *pb <= 'F') )
		{
			if ( pseq->width == 0 )
			{
				do
				{
					parmval *= 16;
					if (*pb >= '0' && *pb <= '9')
						parmval += ( *pb - '0');
					else
						parmval += ( *pb - 'A' + 10 );
					pb++;
					buffno--;
					if ( buffno <= 0 )
					{
						Partial = 1;
						return( NULL );
					}
				} while (  (*pb >= '0' && *pb <= '9')
					|| (*pb >= 'A' && *pb <= 'F') );
			}
			else
			{
				parmval *= 16;
				if (*pb >= '0' && *pb <= '9')
					parmval += ( *pb - '0');
				else
					parmval += ( *pb - 'A' + 10 );
				pb++;
				buffno--;
				for ( i = 1; i < pseq->width; i++ )
				{
					if ( buffno <= 0 )
					{
						Partial = 1;
						return( NULL );
					}
					if (  (*pb < '0' || *pb > '9')
					   && (*pb < 'A' || *pb > 'F') )
						return( NULL );
					parmval *= 16;
					if (*pb >= '0' && *pb <= '9')
						parmval += ( *pb - '0');
					else
						parmval += ( *pb - 'A' + 10 );
					pb++;
					buffno--;
				} 
			}
			Dec_parm[ parmno ] =
				parmval + pseq->offset;
		}
		else
			return( NULL );
		break;

	    /**********************************************************
	    * Look for a string of decimal digits of max length "width"
	    * that will indicate how long a subsequent string is.
	    **********************************************************/
	    case LENGTH_PARM:
		parmval = 0;
		if ( *pb >= '0' && *pb <= '9' )
		{
			if ( pseq->width == 0 )
			{
				do
				{
					parmval *= 10;
					parmval += ( *pb - '0');
					pb++;
					buffno--;
					if ( buffno <= 0 )
					{
						Partial = 1;
						return( NULL );
					}
				} while ( *pb >= '0' && *pb <= '9' );
			}
			else
			{
				parmval *= 10;
				parmval += ( *pb - '0');
				pb++;
				buffno--;
				for ( i = 1; i < pseq->width; i++ )
				{
					if ( buffno <= 0 )
					{
						Partial = 1;
						return( NULL );
					}
					if ( *pb < '0' || *pb > '9' )
						return( NULL );
					parmval *= 10;
					parmval += ( *pb - '0');
					pb++;
					buffno--;
				} 
			}
			Dec_length[ parmno ] =
				parmval + pseq->offset;
		}
		else
			return( NULL );
		break;

	    /**********************************************************
	    * Look for a string of hex digits of max length "width"
	    * that will indicate how long a subsequent string is.
	    **********************************************************/
	    case HEX_LENGTH_PARM:
		parmval = 0;
		if (  ( *pb >= '0' && *pb <= '9' )
		   || ( *pb >= 'A' && *pb <= 'F' ) )
		{
			if ( pseq->width == 0 )
			{
				do
				{
					parmval *= 16;
					if (*pb >= '0' && *pb <= '9')
						parmval += ( *pb - '0');
					else
						parmval += ( *pb - 'A' + 10 );
					pb++;
					buffno--;
					if ( buffno <= 0 )
					{
						Partial = 1;
						return( NULL );
					}
				} while (  (*pb >= '0' && *pb <= '9')
					|| (*pb >= 'A' && *pb <= 'F') );
			}
			else
			{
				parmval *= 16;
				if (*pb >= '0' && *pb <= '9')
					parmval += ( *pb - '0');
				else
					parmval += ( *pb - 'A' + 10 );
				pb++;
				buffno--;
				for ( i = 1; i < pseq->width; i++ )
				{
					if ( buffno <= 0 )
					{
						Partial = 1;
						return( NULL );
					}
					if (  (*pb < '0' || *pb > '9')
					   && (*pb < 'A' || *pb > 'F') )
						return( NULL );
					parmval *= 16;
					if (*pb >= '0' && *pb <= '9')
						parmval += ( *pb - '0');
					else
						parmval += ( *pb - 'A' + 10 );
					pb++;
					buffno--;
				} 
			}
			Dec_length[ parmno ] =
				parmval + pseq->offset;
		}
		else
			return( NULL );
		break;

	    /**********************************************************
	    * Unconditionally matched the next single character.
	    **********************************************************/
	    case BYTE_PARM:
		parms_valid |= parms_valid_bit;
		Dec_parm[ parmno ] = *pb + pseq->offset;
		pb++;
		buffno--;
		break;

	    /**********************************************************
	    * Look for a string of characters of max length "width" that
	    * ends in the character in "offset".
	    **********************************************************/
	    case STRING_PARM:
		string_parms_valid |= parms_valid_bit;
		Dec_string_parm[ parmno ] = string_store;
		if ( pseq->width > 0 )
		{
			if ( Outwin->columns_wide_mode_on == 0 )
				char_count = pseq->width;
			else
				char_count = pseq->width_columns_wide_mode;
		}
		else
			char_count = MAX_BUFF;
		while ( *pb != pseq->offset )
		{
			/************************************************
			* Embedded nulls are ignored by terminal but cause
			* premature ending of null terminated strings.
			* Now everybody ignores this junk.
			************************************************/
			if ( *pb == '\0' )
			{
				pb++;
				buffno--;
				if ( buffno <= 0 )
				{
					*string_store++ = '\0';
					Partial = 1;
					return( NULL );
				}
				continue;
			}
			*string_store++ = *pb++;
			buffno--;
			char_count--;
			if ( char_count <= 0 )
				break;
			if ( buffno <= 0 )
			{
				*string_store++ = '\0';
				Partial = 1;
				return( NULL );
			}
		}
		*string_store++ = '\0';
		if ( char_count > 0 )
		{
			pb++;
			buffno--;
		}
		break;

	    /**************************************************************
	    * Look for a string ending with string pointed to by "string_ptr".
	    * This string is null terminated ans also of length "offset".
	    **************************************************************/
	    case UNTIL_STRING_PARM:
		string_parms_valid |= parms_valid_bit;
		Dec_string_parm[ parmno ] = string_store;
		if ( pseq->width > 0 )
			char_count = pseq->width;
		else
			char_count = MAX_BUFF;
		while (  ( buffno < pseq->offset ) 
		      || ( strncmp( (char *) pb, pseq->string_ptr, 
							    pseq->offset )
			   != 0 )
		      )
		{
			/************************************************
			* Embedded nulls are ignored by terminal but cause
			* premature ending of null terminated strings.
			* Now everybody ignores this junk.
			************************************************/
			if ( *pb == '\0' )
			{
				pb++;
				buffno--;
				if ( buffno < pseq->offset )
				{
					*string_store++ = '\0';
					Partial = 1;
					return( NULL );
				}
				continue;
			}
			*string_store++ = *pb++;
			buffno--;
			char_count--;
			if ( char_count <= 0 )
				return( NULL );
			if ( buffno < pseq->offset )
			{
				*string_store++ = '\0';
				Partial = 1;
				return( NULL );
			}
		}
		*string_store++ = '\0';
		pb += pseq->offset;
		buffno -= pseq->offset;
		break;

	    /**************************************************************
	    * Look for a string that ends in a capital letter.
	    **************************************************************/
	    case HP_STRING_PARM:
		string_parms_valid |= parms_valid_bit;
		Dec_string_parm[ parmno ] = string_store;
		if ( pseq->width > 0 )
			char_count = pseq->width;
		else
			char_count = MAX_BUFF;
		while ( ( *pb < 'A' ) || ( *pb > 'Z' ) )
		{
			*string_store++ = *pb++;
			buffno--;
			char_count--;
			if ( char_count <= 0 )
				return( NULL );	/* exceeding count is error */
			if ( buffno <= 0 )
			{
				*string_store++ = '\0';
				Partial = 1;
				return( NULL );
			}
		}
		*string_store++ = *pb++;
		*string_store++ = '\0';
		pb++;
		buffno--;
		break;

	    /**************************************************************
	    * Look for a string of bytes with the 0x60 field set to 0x20
	    * ending with a byte with the 0x60 field set to 0x40 or 0x60.
	    **************************************************************/
	    case IBM_STRING_PARM:
		string_parms_valid |= parms_valid_bit;
		Dec_string_parm[ parmno ] = string_store;
		if ( pseq->width > 0 )
			char_count = pseq->width;
		else
			char_count = MAX_BUFF;
		while ( ( *pb & 0x60 ) == 0x20 )
		{
			*string_store++ = *pb++;
			buffno--;
			char_count--;
			if ( char_count <= 0 )
				return( NULL );	/* exceeding count is error */
			if ( buffno <= 0 )
			{
				*string_store++ = '\0';
				Partial = 1;
				return( NULL );
			}
		}
		if ( ( ( *pb & 0x60 ) != 0x40 ) && ( ( *pb & 0x60 ) != 0x60 ) )
		{
			*string_store++ = '\0';
			return( NULL );
		}
		*string_store++ = *pb++;
		*string_store++ = '\0';
		pb++;
		buffno--;
		break;

	    /**************************************************************
	    * Look for an ibm decimal number encoded as the bottom 5 bits of
	    * two bytes.
	    * The first byte has 0x40 clear and 0x20 set.
	    * The second is the same unless "offset" is non-zero in which
	    * case the second has 0x40 set and 0x20 clear
	    * Look for a string of bytes ending with 0x40 set and 0x20 clear.
	    **************************************************************/
	    case IBM_DECIMAL_PARM:
		parms_valid |= parms_valid_bit;
		parmval = 0;
		if ( ( *pb & 0x60 ) == 0x20 )
		{
			parmval = *pb & 0x1F;
			parmval <<= 5;
			pb++;
			buffno--;
			if ( buffno <= 0 )
			{
				Partial = 1;
				return( NULL );
			}
			if ( pseq->offset > 0 )
			{
				if ( ( *pb & 0x60 ) != 0x40 )
					return( NULL );
			}
			else
			{
				if ( ( *pb & 0x60 ) != 0x20 )
					return( NULL );
			}
			parmval |= ( *pb & 0x1F );
			pb++;
			buffno--;
			Dec_parm[ parmno ] = parmval;
		}
		else
			return( NULL );
		break;

	    /**********************************************************
	    * Unconditionally matches the next "width" number of characters.
	    **********************************************************/
	    case FIXED_STRING_PARM:
		string_parms_valid |= parms_valid_bit;
		Dec_string_parm[ parmno ] = string_store;
		char_count = pseq->width;
		while ( 1 )
		{
			*string_store++ = *pb++;
			buffno--;
			char_count--;
			if ( char_count <= 0 )
				break;
			if ( buffno <= 0 )
			{
				*string_store++ = '\0';
				Partial = 1;
				return( NULL );
			}
		}
		*string_store++ = '\0';
		break;

	    /**********************************************************
	    * Matches a string that has a delimiter as its first character
	    * and then continues until the same delimiter appears.
	    * Max length "width".
	    **********************************************************/
	    case DELIM_STRING_PARM:
		parms_valid |= ( 1 << POS_FUNCTION_KEY_DELIMITER );
		delim = *pb++;
		buffno--;
		Dec_parm[ POS_FUNCTION_KEY_DELIMITER ] = delim;
		string_parms_valid |= parms_valid_bit;
		Dec_string_parm[ parmno ] = string_store;
		if ( buffno <=0 )
		{
			*string_store++ = '\0';
			Partial = 1;
			return( NULL );
		}
		if ( pseq->width > 0 )
			char_count = pseq->width;
		else
			char_count = MAX_BUFF;
		while ( *pb != delim ) 
		{
			*string_store++ = *pb++;
			buffno--;
			char_count--;
			if ( char_count <= 0 )
				break;
			if ( buffno <= 0 )
			{
				*string_store++ = '\0';
				Partial = 1;
				return( NULL );
			}
		}
		*string_store++ = '\0';
		if ( char_count > 0 )
		{
			pb++;
			buffno--;
		}
		break;

	    /**********************************************************
	    * String of characters whose length was determined earlier
	    * in the sequence in a LENGTH_PARM.
	    **********************************************************/
	    case LENGTH_STRING_PARM:
		string_parms_valid |= parms_valid_bit;
		Dec_string_parm[ parmno ] = string_store;
		char_count = Dec_length[ parmno ];
		while ( 1 )
		{
			*string_store++ = *pb++;
			buffno--;
			char_count--;
			if ( char_count <= 0 )
				break;
			if ( buffno <= 0 )
			{
				*string_store++ = '\0';
				Partial = 1;
				return( NULL );
			}
		}
		*string_store++ = '\0';
		break;

	    /**********************************************************
	    * Look for a single character in "offset" that must be stored
	    * and returned as a string.
	    **********************************************************/
	    case CHARACTER_PARM:
		if ( *pb == pseq->offset )
		{
			string_parms_valid |= parms_valid_bit;
			Dec_string_parm[ parmno ] = string_store;
			*string_store++ = *pb;
			*string_store++ = '\0';
			pb++;
			buffno--;
		}
		else
			return( NULL );
		break;

	    /**********************************************************
	    * Look for a decimal number optionally preceeded by a character
	    * specified in "offset".
	    * Max width is "width".
	    **********************************************************/
	    case MODE_PARM:
		if ( *pb == pseq->offset )
		{
			string_parms_valid |= parms_valid_bit;
			Dec_string_parm[ parmno ] = string_store;
			*string_store++ = *pb;
			*string_store++ = '\0';
			pb++;
			buffno--;
		}
		if ( buffno <= 0 )
		{
			Partial = 1;
			return( NULL );
		}
		parms_valid |= parms_valid_bit;
		parmval = 0;
		if ( *pb >= '0' && *pb <= '9' )
		{
			if ( pseq->width == 0 )
			{
				do
				{
					parmval *= 10;
					parmval += ( *pb - '0');
					pb++;
					buffno--;
					if ( buffno <= 0 )
					{
						Partial = 1;
						return( NULL );
					}
				} while ( *pb >= '0' && *pb <= '9' );
			}
			else
			{
				parmval *= 10;
				parmval += ( *pb - '0');
				pb++;
				buffno--;
				for ( i = 1; i < pseq->width; i++ )
				{
					if ( buffno <= 0 )
					{
						Partial = 1;
						return( NULL );
					}
					if ( *pb < '0' || *pb > '9' )
						return( NULL );
					parmval *= 10;
					parmval += ( *pb - '0');
					pb++;
					buffno--;
				} 
			}
			Dec_parm[ parmno ] = parmval;
		}
		else
			return( NULL );
		break;
	    }				/* end switch */
	}
	else if ( code == *pb )
	{
		/**********************************************************
		* Literal character match
		**********************************************************/
		pb++;
		buffno--;
	}
	else
		return( NULL );
	if ( pseq->next == NULL )
	{					/* matched */
		/**********************************************************
		* Matched the sequence - return it for execution.
		**********************************************************/
		Dec_parms_valid = parms_valid;
		Dec_string_parms_valid = string_parms_valid;
		return( pseq );
	}
	else if ( buffno <= 0 )
	{					/* good so far - out of chars */
		if ( pseq->decode_type )
			Outdecode->decode_type = pseq->decode_type;
		Partial = 1;
		return( NULL );
	}
	else
	{					/* good so far - try next */
		return( check_alsos( pseq->next,
				     parms_valid, pb, buffno,
				     string_store, string_parms_valid )
		      );
	}
}
char *dec_encode();
char *temp_encode();
/**************************************************************************
* extra_decode_type
*	TERMINAL DESCRIPTION PARSER module for "decode_type".
**************************************************************************/
/*ARGSUSED*/
extra_decode_type( buff, string, attr_on_string, attr_off_string ) 
	char	*buff;
	char	*string;
	char	*attr_on_string;
	char	*attr_off_string;
{
	char	storage[ 256 ];

	if ( strcmp( buff, "decode_type" ) == 0 )
	{
		if ( Decode_type_no[ X_pe ] < DECODE_TYPE_MAX )
		{
			temp_encode( string, storage );
			install_decode_type( Decode_type_no[ X_pe ] + 1,
					     (unsigned char *) storage,
					     Seq_root[ X_pe ] );
			temp_encode( attr_on_string, storage );
			install_decode_type_middle( Decode_type_no[ X_pe ],
						    (unsigned char * ) storage,
						    1 );
			Decode_type_no[ X_pe ]++;
		}
		else
		{
				printf( "Too many decode_type\n" );
		}
	}
	else if ( strcmp( buff, "allow_embedded_transparent_print_off" ) == 0 )
	{
		F_allow_embedded_transparent_print_off =
						get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "transparent_print_hold_if_partial" ) == 0 )
	{
		T_transparent_print_hold_if_partial = dec_encode( string );
		Len_transparent_print_hold_if_partial = 
				strlen( T_transparent_print_hold_if_partial );
	}
	else
	{
		return( 0 );		/* no match */
	}
	return( 1 );
}
#ifdef lint
static int lint_alignment_warning_ok_2;
#endif
