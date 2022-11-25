/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: clear.c,v 70.1 92/03/09 15:41:03 ssa Exp $ */
/* #include "stdio.h" */
#include "person.h"
#include "clear_p.h"
#include "ftchar.h"
#include "ftproc.h"
#include "ftterm.h"
#include "ftwindow.h"
#include "decode.h"
#include "features.h"
#include "myattrs.h"
#define MAX_CLEAR_SCREEN 10
int	sT_clear_screen_no = { 0 };
char	*sT_clear_screen[ MAX_CLEAR_SCREEN ] = { (char *) 0 };
				/* attributes turned off by this clear */
FTCHAR	sF_clear_screen_ftattrs_off[ MAX_CLEAR_SCREEN ] = { 0 }; /* attr bits */
				/* 0 = not an unprotected clear
				 * otherwise the attributes that are protected
				 * which defaults to ATTR_PROTECT
				 */
FTCHAR	sF_clear_screen_unprotected[ MAX_CLEAR_SCREEN ] = { 0 };
FTCHAR	sF_clear_screen_w_attr[ MAX_CLEAR_SCREEN ] = { 0 };
int	sF_memory_below = { 0 };
#include "extra.h"
char	*sT_out_clear_screen = { (char *) 0 };
char	*sT_clear_all = { (char *) 0 };
char	*sT_clear_all_w_attr = { (char *) 0 };
FTCHAR	sT_clear_all_w_attr_mask = { ATTR_PROTECT };
char	*sT_clear_all_chars_erasable = { (char *) 0 };
FTCHAR	sT_clear_all_chars_erasable_attr = { 0 };
char	*sT_clr_eol = { (char *) 0 };
char	*sT_out_clr_memory_below = { (char *) 0 };
char	*sT_out_clr_eol = { (char *) 0 };
char	*sT_clr_eol_w_attr = { (char *) 0 };
FTCHAR	sT_clr_eol_w_attr_mask = { ATTR_PROTECT };
char	*sT_out_clr_eol_w_attr = { (char *) 0 };
char	*sT_clr_eol_chars = { (char *) 0 };
char	*T_set_attr_eol = (char *) 0;
char	*sT_clr_eol_chars_erasable = { (char *) 0 };	/* not implemented */
char	*sT_out_clr_eol_chars_erasable = { (char *) 0 };
char	*sT_clr_eol_unprotected = { (char *) 0 };
FTCHAR	sT_clr_eol_unprotected_attr = { ATTR_PROTECT };
char	*sT_out_clr_eol_unprotected = { (char *) 0 };
char	*sT_clr_eol_unprotected_w_attr = { (char *) 0 };
FTCHAR	sT_clr_eol_unprotected_w_attr_attr = { ATTR_PROTECT };
FTCHAR	sT_clr_eol_unprotected_w_attr_mask = { ATTR_PROTECT };
char	*sT_out_clr_eol_unprotected_w_attr = { (char *) 0 };
char	*sT_clr_fld_unprotected = { (char *) 0 };
FTCHAR	sT_clr_fld_unprotected_attr = { ATTR_PROTECT };
char	*sT_out_clr_fld_unprotected = { (char *) 0 };
char	*sT_clr_fld_unprotected_w_attr = { (char *) 0 };
FTCHAR	sT_clr_fld_unprotected_w_attr_attr = { ATTR_PROTECT };
FTCHAR	sT_clr_fld_unprotected_w_attr_mask = { ATTR_PROTECT };
char	*sT_out_clr_fld_unprotected_w_attr = { (char *) 0 };
char	*sT_clr_bol_chars_erasable = { (char *) 0 };	/* not implemented */
char	*sT_out_clr_bol_chars_erasable = { (char *) 0 };
char	*sT_clr_bol = { (char *) 0 };
char	*sT_out_clr_bol = { (char *) 0 };
char	*sT_clr_bol_w_attr = { (char *) 0 };
char	*sT_clr_bol_chars = { (char *) 0 };
char	*T_set_attr_bol = { (char *) 0 };
char	*sT_clear_line = { (char *) 0 };
char	*sT_out_clear_line = { (char *) 0 };
char	*sT_clear_line_w_attr = { (char *) 0 };
char	*sT_clear_line_chars = { (char *) 0 };
char	*T_set_attr_line = { (char *) 0 };
char	*sT_clr_eos = { (char *) 0 };
char	*sT_out_clr_eos = { (char *) 0 };
int	sF_clr_eos_split_screen_with_refresh = { 0 };
char	*sT_clr_eos_w_attr = { (char *) 0 };
FTCHAR	sT_clr_eos_w_attr_mask = { ATTR_PROTECT };
char	*sT_clr_eos_chars = { (char *) 0 };
char	*T_set_attr_eos = { (char *) 0 };
char	*sT_clr_eos_chars_erasable = { (char *) 0 };
FTCHAR	sT_clr_eos_chars_erasable_attr = { 0 };
char	*sT_clr_eos_unprotected = { (char *) 0 };
FTCHAR	sT_clr_eos_unprotected_attr = { ATTR_PROTECT };
char	*sT_clr_eos_unprotected_w_attr = { (char *) 0 };
FTCHAR	sT_clr_eos_unprotected_w_attr_attr = { ATTR_PROTECT };
FTCHAR	sT_clr_eos_unprotected_w_attr_mask = { ATTR_PROTECT };

char	*sT_clr_bos = { (char *) 0 };
char	*sT_clr_bos_chars_erasable = { (char *) 0 };
FTCHAR	sT_clr_bos_chars_erasable_attr = { 0 };

/**************************************************************************
* dec_clear_screen
*	DECODE module for 'clear_screen'.
*	"clear_screen_no" is the occurrence number in the terminal
*	description file of this sequence for getting particular information
*	about this sequence.
**************************************************************************/
/*ARGSUSED*/
dec_clear_screen( clear_screen_no, parm_ptr, parms_valid, parm )
	int	clear_screen_no;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
{
	char	blank_char;

	if ( parms_valid & 1 )
		blank_char = parm[ 0 ];
	else
		blank_char = ' ';
	fct_clear_screen( clear_screen_no, blank_char );
}
/**************************************************************************
* inst_clear_screen
*	INSTALL module for 'clear_screen'.
**************************************************************************/
inst_clear_screen( str, clear_screen_no )
	char	*str;
	int	clear_screen_no;
{
	dec_install( "clear_screen", (UNCHAR *) str, 
					dec_clear_screen, clear_screen_no, 
					CURSOR_OPTION,
					(char *) 0 );
}
/**************************************************************************
* dec_clear_all
*	DECODE module for 'clear_all'.
*	clear all does not home cursor - E.G. vt220 \E[2J
**************************************************************************/
dec_clear_all()
{
	fct_clear_all();
}
/**************************************************************************
* inst_clear_all
*	INSTALL module for 'clear_all'.
**************************************************************************/
inst_clear_all( str )
	char	*str;
{
	dec_install( "clear_all", (UNCHAR *) str, dec_clear_all, 0, 0,
	(char *) 0 );
}
/**************************************************************************
* dec_clear_all_w_attr
*	DECODE module for 'clear_all_w_attr'.
*	clear_all_w_attr does not home cursor - E.G. vt220 \E[2J
**************************************************************************/
dec_clear_all_w_attr()
{
	fct_clear_all_w_attr();
}
/**************************************************************************
* inst_clear_all_w_attr
*	INSTALL module for 'clear_all_w_attr'.
**************************************************************************/
inst_clear_all_w_attr( str )
	char	*str;
{
	dec_install( "clear_all_w_attr", (UNCHAR *) str,
		     dec_clear_all_w_attr, 0, 0,
		     (char *) 0 );
}
/**************************************************************************
* dec_clear_all_chars_erasable
*	DECODE module for 'clear_all_chars_erasable'.
**************************************************************************/
dec_clear_all_chars_erasable()
{
	fct_clear_all_chars_erasable();
}
/**************************************************************************
* inst_clear_all_chars_erasable
*	INSTALL module for 'clear_all_chars_erasable'.
**************************************************************************/
inst_clear_all_chars_erasable( str )
	char	*str;
{
	dec_install( "clear_all_chars_erasable", (UNCHAR *) str, 
				dec_clear_all_chars_erasable, 0, 0,
				(char *) 0 );
}
/**************************************************************************
* make_out_clear_screen
*	Construct a string that when fed to the decoder will clear the
*	screen.  This is used for "clear_window_on_open" and 
*	"clear_window_on_close".  Padding must be removed.
**************************************************************************/
make_out_clear_screen( string, max )
	char	*string;
	int	max;
{
	int	len;

	if ( oT_out_clear_screen != (char *) 0 )
		nopad_strncpy( string, oT_out_clear_screen, max );
	else
	{
		make_out_cursor_home( string, max );
		len = strlen( string );
		if ( make_out_clear_all( &string[ len ], max - len ) != 0 )
			make_out_clr_eos( &string[ len ], max - len );
	}
	return( 0 );
}
/**************************************************************************
* out_term_clear_screen
*	Internally generated_command.
*	TERMINAL OUTPUT module for 'clear_screen'.
**************************************************************************/
out_term_clear_screen()
{
	int	affcnt;			/* needs to be passed in ??? */

	affcnt = Rows;
					/* TBD - this assumes it is not a 
					   clear screen to parameterized char
					   - a safe assumption.
					*/
	if ( mT_out_clear_screen != (char *) 0 )
		my_tputs( mT_out_clear_screen, affcnt );
	else
	{
		term_cursor_home();
		if ( mT_clear_all != (char *) 0 )
			term_clear_all();
		else if ( mT_clear_all_w_attr != (char *) 0 )
			term_clear_all_w_attr();
		else
			out_term_clr_eos( 0 );	/* affrow */
	}
}
/**************************************************************************
* term_clear_screen
*	TERMINAL OUTPUT module for 'clear_screen'.
*	"clear_screen_no" indicates which clear from the terminal 
*	description file.
*	"blank_char" is an optional character used by some clears.
**************************************************************************/
term_clear_screen( clear_screen_no, blank_char )
	int	clear_screen_no;
	char	blank_char;
{
	int	affcnt;			/* needs to be passed in ??? */
	char	*clear_screen_str;
	int	parm[ 2 ];			/* 1 used - must be >= 2 - %i */
	char	*string_parm[ 1 ];		/* not used */
	char	*my_tparm();
	char	*p;

	affcnt = Rows;
	if ( clear_screen_no < mT_clear_screen_no )
	{
		clear_screen_str = mT_clear_screen[ clear_screen_no ];
		if ( clear_screen_str != (char *) 0 )
		{
			parm[ 0 ] = blank_char;
			p = my_tparm( clear_screen_str, parm, string_parm, -1 );
			my_tputs( p, affcnt );
		}
		else
			my_tputs( "CLEAR_SCREEN", affcnt );
	}
	else
		my_tputs( "CLEAR_SCREEN", affcnt );
}
/**************************************************************************
* make_out_clear_all
*	Construct a string that when fed to the decoder will cause a
*	"clear_all" to be done.  See make_out_clear_screen.
**************************************************************************/
make_out_clear_all( string, max )
	char	*string;
	int	max;
{
	if ( oT_clear_all != (char *) 0 )
	{
		nopad_strncpy( string, oT_clear_all, max );
		return( 0 );
	}
	else if ( oT_clear_all_w_attr != (char *) 0 )
	{
		nopad_strncpy( string, oT_clear_all_w_attr, max );
		return( 0 );
	}
	else
		return( -1 );
}
/**************************************************************************
* term_clear_all
*	TERMINAL OUTPUT module for 'clear_all'.
**************************************************************************/
term_clear_all()
{
	int	affcnt;			/* needs to be passed in ??? */

	affcnt = Rows;
	if ( mT_clear_all != (char *) 0 )
		my_tputs( mT_clear_all, affcnt );
}
/**************************************************************************
* term_clear_all_w_attr
*	TERMINAL OUTPUT module for 'clear_all_w_attr'.
**************************************************************************/
term_clear_all_w_attr()
{
	int	affcnt;			/* needs to be passed in ??? */

	affcnt = Rows;
	if ( mT_clear_all_w_attr != (char *) 0 )
		my_tputs( mT_clear_all_w_attr, affcnt );
}
/**************************************************************************
* term_clear_all_chars_erasable
*	TERMINAL OUTPUT module for 'clear_all_chars_erasable'.
**************************************************************************/
term_clear_all_chars_erasable()
{
	int	affcnt;			/* needs to be passed in ??? */

	affcnt = Rows;
	if ( mT_clear_all_chars_erasable != (char *) 0 )
		my_tputs( mT_clear_all_chars_erasable, affcnt );
}
/**************************************************************************
* out_term_clr_memory_below
*	If "out_term_clr_memory_below" was present in terminal_description
*	then output it and return 0.
*	Otherwise return -1.
*	This is used to prevent characters below from invading the screen
*	on a window switch.
**************************************************************************/
out_term_clr_memory_below()
{
	if ( mT_out_clr_memory_below != (char *) 0 )
	{
		my_tputs( mT_out_clr_memory_below, 1 );
		return( -1 );
	}
	else
		return( -1 );
}
/**************************************************************************
* out_term_clr_eol
*	Internally generated clear to end of line.
**************************************************************************/
out_term_clr_eol()
{
	if ( mT_out_clr_eol != (char *) 0 )
		my_tputs( mT_out_clr_eol, 1 );
	else if ( mT_clr_eol != (char *) 0 )
		my_tputs( mT_clr_eol, 1 );
	else if ( mT_out_clr_eol_w_attr != (char *) 0 )
		my_tputs( mT_out_clr_eol_w_attr, 1 );
	else if ( mT_clr_eol_w_attr != (char *) 0 )
		my_tputs( mT_clr_eol_w_attr, 1 );
	else if ( mT_out_clr_eol_unprotected != (char *) 0 )
		my_tputs( mT_out_clr_eol_unprotected, 1 );
	else if ( mT_clr_eol_unprotected != (char *) 0 )
		my_tputs( mT_clr_eol_unprotected, 1 );
	else if ( mT_clr_eol_unprotected_w_attr != (char *) 0 )
		my_tputs( mT_clr_eol_unprotected_w_attr, 1 );
	else if ( mT_out_clr_fld_unprotected != (char *) 0 )
		my_tputs( mT_out_clr_fld_unprotected, 1 );
	else if ( mT_clr_fld_unprotected != (char *) 0 )
		my_tputs( mT_clr_fld_unprotected, 1 );
	else if ( mT_clr_fld_unprotected_w_attr != (char *) 0 )
		my_tputs( mT_clr_fld_unprotected_w_attr, 1 );
}
/**************************************************************************
* term_clr_eol
*	TERMINAL OUTPUT module for 'clr_eol'.
**************************************************************************/
term_clr_eol()
{
	if ( mT_clr_eol != (char *) 0 )
		my_tputs( mT_clr_eol, 1 );
	else
		term_clr_eol_w_attr();
}
/**************************************************************************
* out_term_clr_eol_w_attr
*	Internally generated clear to end of line with characters of
*	current attribute.
**************************************************************************/
out_term_clr_eol_w_attr()
{
	if ( mT_out_clr_eol_w_attr != (char *) 0 )
		my_tputs( mT_out_clr_eol_w_attr, 1 );
	else
		term_clr_eol_w_attr();
}
/**************************************************************************
* term_clr_eol_w_attr
*	TERMINAL OUTPUT module for 'clr_eol_w_attr'.
**************************************************************************/
term_clr_eol_w_attr()
{
	if ( mT_clr_eol_w_attr != (char *) 0 )
		my_tputs( mT_clr_eol_w_attr, 1 );
}
/**************************************************************************
* term_clr_eol_chars
*	TERMINAL OUTPUT module for 'clr_eol_chars'.
**************************************************************************/
term_clr_eol_chars()
{
	if ( mT_clr_eol_chars != (char *) 0 )
		my_tputs( mT_clr_eol_chars, 1 );
}
/**************************************************************************
* term_set_attr_eol
*	TERMINAL OUTPUT module for 'set_attr_eol'.
**************************************************************************/
term_set_attr_eol()
{
	if ( T_set_attr_eol != (char *) 0 )
		my_tputs( T_set_attr_eol, 1 );
}
/**************************************************************************
* out_term_clr_eol_chars_erasable
*	Internally generated clear to end of line erasable chars.
**************************************************************************/
out_term_clr_eol_chars_erasable()
{
	if ( mT_out_clr_eol_chars_erasable != (char *) 0 )
		my_tputs( mT_out_clr_eol_chars_erasable, 1 );
	else
		term_clr_eol_chars_erasable();
}
/**************************************************************************
* term_clr_eol_chars_erasable
*	TERMINAL OUTPUT module for 'clr_eol_chars_erasable'.
**************************************************************************/
term_clr_eol_chars_erasable()
{
	if ( mT_clr_eol_chars_erasable != (char *) 0 )
		my_tputs( mT_clr_eol_chars_erasable, 1 );
}
/**************************************************************************
* out_term_clr_eol_unprotected
*	Internally generated clear to end of line unprotected.
**************************************************************************/
out_term_clr_eol_unprotected()
{
	if ( mT_out_clr_eol_unprotected != (char *) 0 )
		my_tputs( mT_out_clr_eol_unprotected, 1 );
	else
		term_clr_eol_unprotected();
}
/**************************************************************************
* term_clr_eol_unprotected
*	TERMINAL OUTPUT module for 'clr_eol_unprotected'.
**************************************************************************/
term_clr_eol_unprotected()
{
	if ( mT_clr_eol_unprotected != (char *) 0 )
		my_tputs( mT_clr_eol_unprotected, 1 );
}
/**************************************************************************
* out_term_clr_eol_unprotected_w_attr
*	Internally generated clear to end of line unprotected.
**************************************************************************/
out_term_clr_eol_unprotected_w_attr()
{
	if ( mT_out_clr_eol_unprotected_w_attr != (char *) 0 )
		my_tputs( mT_out_clr_eol_unprotected_w_attr, 1 );
	else
		term_clr_eol_unprotected_w_attr();
}
/**************************************************************************
* term_clr_eol_unprotected_w_attr
*	TERMINAL OUTPUT module for 'clr_eol_unprotected_w_attr'.
**************************************************************************/
term_clr_eol_unprotected_w_attr()
{
	if ( mT_clr_eol_unprotected_w_attr != (char *) 0 )
		my_tputs( mT_clr_eol_unprotected_w_attr, 1 );
}
/**************************************************************************
* out_term_clr_fld_unprotected
*	Internally generated clear to end of line unprotected.
**************************************************************************/
out_term_clr_fld_unprotected()
{
	if ( mT_out_clr_fld_unprotected != (char *) 0 )
		my_tputs( mT_out_clr_fld_unprotected, 1 );
	else
		term_clr_fld_unprotected();
}
/**************************************************************************
* term_clr_fld_unprotected
*	TERMINAL OUTPUT module for 'clr_fld_unprotected'.
**************************************************************************/
term_clr_fld_unprotected()
{
	if ( mT_clr_fld_unprotected != (char *) 0 )
		my_tputs( mT_clr_fld_unprotected, 1 );
}
/**************************************************************************
* out_term_clr_fld_unprotected_w_attr
*	Internally generated clear to end of line unprotected.
**************************************************************************/
out_term_clr_fld_unprotected_w_attr()
{
	if ( mT_out_clr_fld_unprotected_w_attr != (char *) 0 )
		my_tputs( mT_out_clr_fld_unprotected_w_attr, 1 );
	else
		term_clr_fld_unprotected_w_attr();
}
/**************************************************************************
* term_clr_fld_unprotected_w_attr
*	TERMINAL OUTPUT module for 'clr_fld_unprotected_w_attr'.
**************************************************************************/
term_clr_fld_unprotected_w_attr()
{
	if ( mT_clr_fld_unprotected_w_attr != (char *) 0 )
		my_tputs( mT_clr_fld_unprotected_w_attr, 1 );
}
/**************************************************************************
* out_term_clr_bol
*	Internally generated clear to beginning of line.
**************************************************************************/
out_term_clr_bol()
{
	if ( mT_out_clr_bol != (char *) 0 )
		my_tputs( mT_out_clr_bol, 1 );
	else
		term_clr_bol();
}
/**************************************************************************
* term_clr_bol
*	TERMINAL OUTPUT module for 'clr_bol'.
**************************************************************************/
term_clr_bol()
{
	if ( mT_clr_bol != (char *) 0 )
		my_tputs( mT_clr_bol, 1 );
	else
		term_clr_bol_w_attr();
}
/**************************************************************************
* term_clr_bol_w_attr
*	TERMINAL OUTPUT module for 'clr_bol_w_attr'.
*	clear to beginning of line with attribute.
**************************************************************************/
term_clr_bol_w_attr()
{
	if ( mT_clr_bol_w_attr != (char *) 0 )
		my_tputs( mT_clr_bol_w_attr, 1 );
}
/**************************************************************************
* term_clr_bol_chars
*	TERMINAL OUTPUT module for 'clr_bol_chars'.
**************************************************************************/
term_clr_bol_chars()
{
	if ( mT_clr_bol_chars != (char *) 0 )
		my_tputs( mT_clr_bol_chars, 1 );
}
/**************************************************************************
* term_set_attr_bol
*	TERMINAL OUTPUT module for 'set_attr_bol'.
**************************************************************************/
term_set_attr_bol()
{
	if ( T_set_attr_bol != (char *) 0 )
		my_tputs( T_set_attr_bol, 1 );
}
/**************************************************************************
* out_term_clr_bol_chars_erasable
*	Internally generated clear to beginning of line all erasable chars.
**************************************************************************/
out_term_clr_bol_chars_erasable()
{
	if ( mT_out_clr_bol_chars_erasable != (char *) 0 )
		my_tputs( mT_out_clr_bol_chars_erasable, 1 );
	else
		term_clr_bol_chars_erasable();
}
/**************************************************************************
* term_clr_bol_chars_erasable
*	TERMINAL OUTPUT module for 'clr_bol_chars_erasable'.
**************************************************************************/
term_clr_bol_chars_erasable()
{
	if ( mT_clr_bol_chars_erasable != (char *) 0 )
		my_tputs( mT_clr_bol_chars_erasable, 1 );
}
/**************************************************************************
* out_term_clear_line
*	Internally generated clear line.
**************************************************************************/
out_term_clear_line()
{
	if ( mT_out_clear_line != (char *) 0 )
		my_tputs( mT_out_clear_line, 1 );
	else
		term_clear_line();
}
/**************************************************************************
* term_clear_line
*	TERMINAL OUTPUT module for 'clear_line'.
**************************************************************************/
term_clear_line()
{
	if ( mT_clear_line != (char *) 0 )
		my_tputs( mT_clear_line, 1 );
	else
		term_clear_line_w_attr();
}
/**************************************************************************
* term_clear_line_w_attr
*	TERMINAL OUTPUT module for 'clear_line_w_attr'.
*	clear line with blanks of the current attribute.
**************************************************************************/
term_clear_line_w_attr()
{
	if ( mT_clear_line_w_attr != (char *) 0 )
		my_tputs( mT_clear_line_w_attr, 1 );
}
/**************************************************************************
* term_clear_line_chars
*	TERMINAL OUTPUT module for 'clear_line_chars'.
**************************************************************************/
term_clear_line_chars()
{
	if ( mT_clear_line_chars != (char *) 0 )
		my_tputs( mT_clear_line_chars, 1 );
}
/**************************************************************************
* term_set_attr_line
*	TERMINAL OUTPUT module for 'set_attr_line'.
**************************************************************************/
term_set_attr_line()
{
	if ( T_set_attr_line != (char *) 0 )
		my_tputs( T_set_attr_line, 1 );
}
/**************************************************************************
* make_out_clr_eos
*	Create a string that when fed to the decoder will do a 
*	"clear to end of screen". See make_out_clear_screen.
**************************************************************************/
make_out_clr_eos( string, max )
	char	*string;
	int	max;
{
	if ( oT_out_clr_eos != (char *) 0 )
		nopad_strncpy( string, oT_out_clr_eos, max );
	else if ( oT_clr_eos != (char *) 0 )
		nopad_strncpy( string, oT_clr_eos, max );
	else if ( oT_clr_eos_w_attr != (char *) 0 )
		nopad_strncpy( string, oT_clr_eos_w_attr, max );
	else if ( oT_clr_eos_unprotected != (char *) 0 )
		nopad_strncpy( string, oT_clr_eos_unprotected, max );
	else if ( oT_clr_eos_unprotected_w_attr != (char *) 0 )
		nopad_strncpy( string, oT_clr_eos_unprotected_w_attr, max );
	else
		return( -1 );
	return( 0 );
}
/**************************************************************************
* out_term_clr_eos
*	Internally generated clear to end of screen.
**************************************************************************/
out_term_clr_eos( affrow )
	int	affrow;			/* top row affected for delays */
{
	int	affcnt;	

	affcnt = Rows - affrow;
	if ( mT_out_clr_eos != (char *) 0 )
		my_tputs( mT_out_clr_eos, affcnt );
	else if ( mT_clr_eos != (char *) 0 )
		my_tputs( mT_clr_eos, affcnt );
	else if ( mT_clr_eos_w_attr != (char *) 0 )
		my_tputs( mT_clr_eos_w_attr, affcnt );
	else if ( mT_clr_eos_unprotected != (char *) 0 )
		my_tputs( mT_clr_eos_unprotected, affcnt );
	else if ( mT_clr_eos_unprotected_w_attr != (char *) 0 )
		my_tputs( mT_clr_eos_unprotected_w_attr, affcnt );
}
/**************************************************************************
* term_clr_eos
*	TERMINAL OUTPUT module for 'clr_eos'.
**************************************************************************/
term_clr_eos( affrow )
	int	affrow;			/* top row affected for delays */
{
	int	affcnt;	

	affcnt = Rows - affrow;
	if ( mT_clr_eos != (char *) 0 )
		my_tputs( mT_clr_eos, affcnt );
	else
		term_clr_eos_w_attr( affrow );
}
/**************************************************************************
* term_clr_eos_w_attr
*	TERMINAL OUTPUT module for 'clr_eos_w_attr'.
**************************************************************************/
term_clr_eos_w_attr( affrow )
	int	affrow;			/* top row affected for delays */
{
	int	affcnt;			/* needs to be passed in ??? */

	affcnt = Rows - affrow;
	if ( mT_clr_eos_w_attr != (char *) 0 )
		my_tputs( mT_clr_eos_w_attr, affcnt );
}
/**************************************************************************
* term_clr_eos_chars
*	TERMINAL OUTPUT module for 'clr_eos_chars'.
**************************************************************************/
term_clr_eos_chars( affrow )
	int	affrow;			/* top row affected for delays */
{
	int	affcnt;			/* needs to be passed in ??? */

	affcnt = Rows - affrow;
	if ( mT_clr_eos_chars != (char *) 0 )
		my_tputs( mT_clr_eos_chars, affcnt );
}
/**************************************************************************
* term_set_attr_eos
*	TERMINAL OUTPUT module for 'set_attr_eos'.
**************************************************************************/
term_set_attr_eos( affrow )
	int	affrow;			/* top row affected for delays */
{
	int	affcnt;			/* needs to be passed in ??? */

	affcnt = Rows - affrow;
	if ( T_set_attr_eos != (char *) 0 )
		my_tputs( T_set_attr_eos, affcnt );
}
/**************************************************************************
* term_clr_eos_chars_erasable
*	TERMINAL OUTPUT module for 'clr_eos_chars_erasable'.
**************************************************************************/
term_clr_eos_chars_erasable( affrow )
	int	affrow;			/* top row affected for delays */
{
	int	affcnt;			/* needs to be passed in ??? */

	affcnt = Rows - affrow;
	if ( mT_clr_eos_chars_erasable != (char *) 0 )
		my_tputs( mT_clr_eos_chars_erasable, affcnt );
}
/**************************************************************************
* term_clr_eos_unprotected
*	TERMINAL OUTPUT module for 'clr_eos_unprotected'.
**************************************************************************/
term_clr_eos_unprotected( affrow )
	int	affrow;			/* top row affected for delays */
{
	int	affcnt;			/* needs to be passed in ??? */

	affcnt = Rows - affrow;
	if ( mT_clr_eos_unprotected != (char *) 0 )
		my_tputs( mT_clr_eos_unprotected, affcnt );
}
/**************************************************************************
* term_clr_eos_unprotected_w_attr
*	TERMINAL OUTPUT module for 'clr_eos_unprotected_w_attr'.
**************************************************************************/
term_clr_eos_unprotected_w_attr( affrow )
	int	affrow;			/* top row affected for delays */
{
	int	affcnt;			/* needs to be passed in ??? */

	affcnt = Rows - affrow;
	if ( mT_clr_eos_unprotected_w_attr != (char *) 0 )
		my_tputs( mT_clr_eos_unprotected_w_attr, affcnt );
}
/**************************************************************************
* term_clr_bos
*	TERMINAL OUTPUT module for 'clr_bos'.
**************************************************************************/
term_clr_bos( affrow )
	int	affrow;			/* top row affected for delays */
{
	int	affcnt;	

	affcnt = affrow;
	if ( mT_clr_bos != (char *) 0 )
		my_tputs( mT_clr_bos, affcnt );
}
/**************************************************************************
* term_clr_bos_chars_erasable
*	TERMINAL OUTPUT module for 'clr_bos_chars_erasable'.
**************************************************************************/
term_clr_bos_chars_erasable( affrow )
	int	affrow;			/* top row affected for delays */
{
	int	affcnt;			/* needs to be passed in ??? */

	affcnt = affrow;
	if ( mT_clr_bos_chars_erasable != (char *) 0 )
		my_tputs( mT_clr_bos_chars_erasable, affcnt );
}
/**************************************************************************
* fct_clr_eol
*	ACTION module for 'clr_eol'.
**************************************************************************/
fct_clr_eol()
{
	int	d_clr_eol();
	int	term_clr_eol();

	fct_clr_eol_common( d_clr_eol, (FTCHAR) 0, (FTCHAR) ' ', term_clr_eol );
}
/**************************************************************************
* fct_clr_eol_w_attr
*	ACTION module for 'clr_eol_w_attr'.
**************************************************************************/
fct_clr_eol_w_attr()
{
	int	d_clr_eol();
	int	term_clr_eol_w_attr();
	FTCHAR  blank_char_w_attr;

	blank_char_w_attr = (FTCHAR) ' ' |
		( Outwin->ftattrs & oT_clr_eol_w_attr_mask );
	fct_clr_eol_common( d_clr_eol, (FTCHAR) 0, blank_char_w_attr,
							term_clr_eol_w_attr );
}
/**************************************************************************
* fct_clr_eol_chars
*	ACTION module for 'clr_eol_chars'.
**************************************************************************/
fct_clr_eol_chars()
{
	int	d_clr_eol_chars();
	int	term_clr_eol_chars();

	fct_clr_eol_common( d_clr_eol_chars, (FTCHAR) 0, (FTCHAR) ' ',
							term_clr_eol_chars );
}
/**************************************************************************
* fct_set_attr_eol
*	ACTION module for 'set_attr_eol'.
**************************************************************************/
fct_set_attr_eol()
{
	int	d_set_attr_eol();
	int	term_set_attr_eol();

	fct_clr_eol_common( d_set_attr_eol, (FTCHAR) 0,
					    (FTCHAR) ' ', /* not used */
					    term_set_attr_eol );
}
/**************************************************************************
* fct_clr_eol_common
*	INSTALL module for 'clr_eol_common'.
**************************************************************************/
fct_clr_eol_common( func_d_clr_eol, func_d_clr_eol_parm, blank_char_w_attr, 
							func_term_clr_eol )
	int	(*func_d_clr_eol)();
	FTCHAR	func_d_clr_eol_parm;
	FTCHAR	blank_char_w_attr;
	int	(*func_term_clr_eol)();
{
	(*func_d_clr_eol)( Outwin->row, Outwin->col, func_d_clr_eol_parm,
							blank_char_w_attr );
	if ( Outwin->onscreen )
		(*func_term_clr_eol)();
}
/**************************************************************************
* fct_clr_eol_unprotected
*	ACTION module for 'clr_eol_unprotected'.
**************************************************************************/
fct_clr_eol_unprotected()
{
	int	d_clr_eol_unprotected();
	int	d_clr_eol();
	int	term_clr_eol_unprotected();

	if ( Outwin->write_protect_on )
		fct_clr_eol_common(	d_clr_eol_unprotected,
					xT_clr_eol_unprotected_attr,
					(FTCHAR) ' ',
					term_clr_eol_unprotected );
	else
		fct_clr_eol_common(	d_clr_eol,
					(FTCHAR) 0,
					(FTCHAR) ' ',
					term_clr_eol_unprotected );
}
/**************************************************************************
* fct_clr_eol_unprotected_w_attr
*	ACTION module for 'clr_eol_unprotected_w_attr'.
**************************************************************************/
fct_clr_eol_unprotected_w_attr()
{
	int	d_clr_eol_unprotected();
	int	d_clr_eol();
	int	term_clr_eol_unprotected_w_attr();
	FTCHAR  blank_char_w_attr;

	blank_char_w_attr = (FTCHAR) ' ' |
		( Outwin->ftattrs & oT_clr_eol_unprotected_w_attr_mask );
	if ( Outwin->write_protect_on )
		fct_clr_eol_common(	d_clr_eol_unprotected,
					xT_clr_eol_unprotected_w_attr_attr,
					blank_char_w_attr,
					term_clr_eol_unprotected_w_attr );
	else
		fct_clr_eol_common(	d_clr_eol,
					(FTCHAR) 0,
					blank_char_w_attr,
					term_clr_eol_unprotected_w_attr );
}
/**************************************************************************
* fct_clr_fld_unprotected
*	ACTION module for 'clr_fld_unprotected'.
**************************************************************************/
fct_clr_fld_unprotected()
{
	int	d_clr_fld_unprotected();
	int	d_clr_eol();
	int	term_clr_fld_unprotected();

	if ( Outwin->write_protect_on )
		fct_clr_eol_common(	d_clr_fld_unprotected,
					xT_clr_fld_unprotected_attr,
					(FTCHAR) ' ',
					term_clr_fld_unprotected );
	else
		fct_clr_eol_common(	d_clr_eol,
					(FTCHAR) 0,
					(FTCHAR) ' ',
					term_clr_fld_unprotected );
}
/**************************************************************************
* fct_clr_fld_unprotected_w_attr
*	ACTION module for 'clr_fld_unprotected_w_attr'.
**************************************************************************/
fct_clr_fld_unprotected_w_attr()
{
	int	d_clr_fld_unprotected();
	int	d_clr_eol();
	int	term_clr_fld_unprotected_w_attr();
	FTCHAR  blank_char_w_attr;

	blank_char_w_attr = (FTCHAR) ' ' |
		( Outwin->ftattrs & oT_clr_fld_unprotected_w_attr_mask );
	if ( Outwin->write_protect_on )
		fct_clr_eol_common(	d_clr_fld_unprotected,
					xT_clr_fld_unprotected_w_attr_attr,
					blank_char_w_attr,
					term_clr_fld_unprotected_w_attr );
	else
		fct_clr_eol_common(	d_clr_eol,
					(FTCHAR) 0,
					blank_char_w_attr,
					term_clr_fld_unprotected_w_attr );
}
/**************************************************************************
* fct_clr_bol
*	ACTION module for 'clr_bol'.
**************************************************************************/
fct_clr_bol()
{
	int	d_clr_bol();
	int	term_clr_bol();

	fct_clr_bol_common( d_clr_bol, term_clr_bol );
}
/**************************************************************************
* fct_clr_bol_w_attr
*	ACTION module for 'clr_bol_w_attr'.
**************************************************************************/
fct_clr_bol_w_attr()
{
	int	d_clr_bol_w_attr();
	int	term_clr_bol_w_attr();

	fct_clr_bol_common( d_clr_bol_w_attr, term_clr_bol_w_attr );
}
/**************************************************************************
* fct_clr_bol_chars
*	ACTION module for 'cls_bol_chars'.
**************************************************************************/
fct_clr_bol_chars()
{
	int	d_clr_bol_chars();
	int	term_clr_bol_chars();

	fct_clr_bol_common( d_clr_bol_chars, term_clr_bol_chars );
}
/**************************************************************************
* fct_set_attr_bol
*	ACTION module for 'set_attr_bol'.
**************************************************************************/
fct_set_attr_bol()
{
	int	d_set_attr_bol();
	int	term_set_attr_bol();

	fct_clr_bol_common( d_set_attr_bol, term_set_attr_bol );
}
/**************************************************************************
* fct_clr_bol_common
*	Common module for clear to beginning of line.
**************************************************************************/
fct_clr_bol_common( func_d_clr_bol, func_term_clr_bol )
	int	(*func_d_clr_bol)();
	int	(*func_term_clr_bol)();
{
	(*func_d_clr_bol)( Outwin->row, Outwin->col );
	if ( Outwin->onscreen )
		(*func_term_clr_bol)();
}
/**************************************************************************
* fct_clear_line
*	ACTION module for 'clear_line'.
**************************************************************************/
fct_clear_line()
{
	int	d_clear_line();
	int	term_clear_line();

	fct_clear_line_common( d_clear_line, term_clear_line );
}
/**************************************************************************
* fct_clear_line_w_attr
*	ACTION module for 'clear_line_w_attr'.
**************************************************************************/
fct_clear_line_w_attr()
{
	int	d_clear_line_w_attr();
	int	term_clear_line_w_attr();

	fct_clear_line_common( d_clear_line_w_attr, term_clear_line_w_attr );
}
/**************************************************************************
* fct_clear_line_chars
*	ACTION module for 'clear_line_chars'.
**************************************************************************/
fct_clear_line_chars()
{
	int	d_clear_line_chars();
	int	term_clear_line_chars();

	fct_clear_line_common( d_clear_line_chars, term_clear_line_chars );
}
/**************************************************************************
* fct_set_attr_line
*	ACTION module for 'set_attr_line'.
**************************************************************************/
fct_set_attr_line()
{
	int	d_set_attr_line();
	int	term_set_attr_line();

	fct_clear_line_common( d_set_attr_line, term_set_attr_line );
}
/**************************************************************************
* fct_clear_line_common
*	Common module for clear line ACTION modules.
**************************************************************************/
fct_clear_line_common( func_d_clear_line, func_term_clear_line )
	int	(*func_d_clear_line)();
	int	(*func_term_clear_line)();
{
	(*func_d_clear_line)( Outwin->row );
	if ( Outwin->onscreen )
		(*func_term_clear_line)();
}
/**************************************************************************
* fct_clr_eos
*	ACTION module for 'clr_eos'.
**************************************************************************/
fct_clr_eos()
{
	int	d_clr_eos();
	int	term_clr_eos();
	int	out_term_clr_eol();

	fct_clr_eos_common( d_clr_eos, (FTCHAR) 0, (FTCHAR) ' ',
			    term_clr_eos, out_term_clr_eol );
}
/**************************************************************************
* fct_clr_eos_w_attr
*	ACTION module for 'clr_eos_w_attr'.
**************************************************************************/
fct_clr_eos_w_attr()
{
	int	d_clr_eos();
	int	term_clr_eos_w_attr();
	int	out_term_clr_eol_w_attr();
	FTCHAR	blank_char_w_attr;

	blank_char_w_attr = (FTCHAR) ' ' |
		( Outwin->ftattrs & oT_clr_eos_w_attr_mask );
	fct_clr_eos_common( d_clr_eos, (FTCHAR) 0, blank_char_w_attr,
			    term_clr_eos_w_attr, out_term_clr_eol_w_attr );
}
/**************************************************************************
* fct_clr_eos_chars
*	ACTION module for 'clr_eos_chars'.
**************************************************************************/
fct_clr_eos_chars()
{
	int	d_clr_eos_chars();
	int	term_clr_eos_chars();
	int	term_clr_eol_chars();

	fct_clr_eos_common( d_clr_eos_chars, (FTCHAR) 0, (FTCHAR) ' ',
			    term_clr_eos_chars, term_clr_eol_chars );
}
/**************************************************************************
* fct_set_attr_eos
*	ACTION module for 'set_attr_eos'.
**************************************************************************/
fct_set_attr_eos()
{
	int	d_set_attr_eos();
	int	term_set_attr_eos();
	int	term_set_attr_eol();

	fct_clr_eos_common( d_set_attr_eos, (FTCHAR) 0, (FTCHAR) ' ',
			    term_set_attr_eos, term_set_attr_eol );
}
/**************************************************************************
* fct_clr_eos_chars_erasable
*	ACTION module for 'clr_eos_chars_erasable'.
**************************************************************************/
fct_clr_eos_chars_erasable()
{
	int	d_clr_eos_chars_erasable();
	int	term_clr_eos_chars_erasable();
	int	out_term_clr_eol_chars_erasable();

	fct_clr_eos_common( d_clr_eos_chars_erasable, 
			    oT_clr_eos_chars_erasable_attr,
			    (FTCHAR) ' ',			/* not used */
			    term_clr_eos_chars_erasable, 
			    out_term_clr_eol_chars_erasable );
}
/**************************************************************************
* fct_clr_eos_unprotected
*	ACTION module for 'clr_eos_unprotected'.
**************************************************************************/
fct_clr_eos_unprotected()
{
	int	d_clr_eos_unprotected();
	int	d_clr_eos();
	int	term_clr_eos_unprotected();
	int	out_term_clr_eol_unprotected();

	if ( Outwin->write_protect_on )
		fct_clr_eos_common(	d_clr_eos_unprotected, 
					oT_clr_eos_unprotected_attr,
					(FTCHAR) ' ',
					term_clr_eos_unprotected, 
					out_term_clr_eol_unprotected );
	else
		fct_clr_eos_common(	d_clr_eos, 
					(FTCHAR) 0,
					(FTCHAR) ' ',
					term_clr_eos_unprotected, 
					out_term_clr_eol_unprotected );
}
/**************************************************************************
* fct_clr_eos_unprotected_w_attr
*	ACTION module for 'clr_eos_unprotected_w_attr'.
**************************************************************************/
fct_clr_eos_unprotected_w_attr()
{
	int	d_clr_eos_unprotected();
	int	d_clr_eos();
	int	term_clr_eos_unprotected_w_attr();
	int	out_term_clr_eol_unprotected_w_attr();
	FTCHAR  blank_char_w_attr;

	blank_char_w_attr = (FTCHAR) ' ' |
		( Outwin->ftattrs & oT_clr_eos_unprotected_w_attr_mask );
	if ( Outwin->write_protect_on )
		fct_clr_eos_common(	d_clr_eos_unprotected, 
					oT_clr_eos_unprotected_w_attr_attr,
					blank_char_w_attr,
					term_clr_eos_unprotected_w_attr, 
					out_term_clr_eol_unprotected_w_attr );
	else
		fct_clr_eos_common(	d_clr_eos, 
					(FTCHAR) 0,
					blank_char_w_attr,
					term_clr_eos_unprotected_w_attr, 
					out_term_clr_eol_unprotected_w_attr );
}
/**************************************************************************
* fct_clr_eos_common
*	Common routine for various flavors of "clear to end of screen".
*	func_d_clr_eos
*		function to do the appropriate clear to end of screen on
*		the window buffer.
*	func_d_clr_eos_parm
*		possible attribute parameter to func_d_clr_eos if its
*		clear is attribute dependent.
*	blank_char_w_attr
*		a blank or a character with or without the appropriate
*		attributes, depending on the flavor of clear to end of screen.
*	func_term_clr_eos
*		function to do the appropriate clear to end of screen on
*		the terminal.
*	func_term_clr_eol
*		a function compatible with func_term_clr_eos that does a
*		clear end of line - for use on split screen.
**************************************************************************/
fct_clr_eos_common( func_d_clr_eos, func_d_clr_eos_parm, blank_char_w_attr,
				func_term_clr_eos, func_term_clr_eol )
	int	(*func_d_clr_eos)();
	FTCHAR	func_d_clr_eos_parm;		/* sometimes not used */
	FTCHAR	blank_char_w_attr;		/* sometimes not used */
	int	(*func_term_clr_eos)();
	int	(*func_term_clr_eol)();
{
	int	affrow;

	(*func_d_clr_eos)( Outwin->row, Outwin->col, func_d_clr_eos_parm,
						     blank_char_w_attr );
	if ( Outwin->onscreen )
	{
		if ( Outwin->fullscreen )
		{
			affrow = Outwin->row;
			(*func_term_clr_eos)( affrow );
		}
		else if ( oF_clr_eos_split_screen_with_refresh )
		{
			modes_off_redo_screen();
			split_refresh();
			s_cursor();
			modes_on_redo_screen();
		}
		else
			s_clr_eos_common( func_term_clr_eol );
	}
}
/**************************************************************************
* s_clr_eos_common
*	split screen version of fct_clr_eos_common
**************************************************************************/
s_clr_eos_common( func_term_clr_eol ) /* screen clr to bot for onscreen split */
	int	(*func_term_clr_eol)();
{
	int	row;

					/* possible partial first line */
	(*func_term_clr_eol)();
	if ( Outwin->col == 0 )
		term_line_attribute( Outwin->line_attribute[ Outwin->row ] );
					/* clear rest of visible lines */
	for ( row = Outwin->win_top_row
		    + Outwin->row + 1
		    - Outwin->buff_top_row;
	      row <= Outwin->win_bot_row;
	      row++ )
	{
		term_pos( row, 0 );
		(*func_term_clr_eol)();
		term_line_attribute( Outwin->line_attribute[ row ] );
	}
	s_cursor();
}
/**************************************************************************
* fct_clear_screen
*	ACTION module for 'clear_screen'.
**************************************************************************/
fct_clear_screen( clear_screen_no, blank_char )
	int	clear_screen_no;
	char	blank_char;		/* a space unless parm in encoded */
{
	int	term_clear_screen();
	int	out_term_clr_eol();
	int	set;
	int	selection;
	FTCHAR	ftattrs_off;
	FTCHAR	w_attr_mask;
	FTCHAR  blank_char_w_attr;

	/******************************************************************
	* Meta roll is cancelled when clear screen occurrs.
	******************************************************************/
	t_se_meta_roll_cancel();
	/******************************************************************
	* If clear screen turns of some attributes or resets some character
	* sets, then do those effects on a split screen because we will
	* not actually output the clear screen sequence.
	* On a full screen, adjust our terminals assumed state.
	******************************************************************/
	ftattrs_off = oF_clear_screen_ftattrs_off[ clear_screen_no ];
	if ( ftattrs_off )
	{
		m_clear_regular( ftattrs_off );
		if ( ftattrs_off & F_character_set_attributes )
		{
			m_sync_character_set_to_attr( Outwin->ftattrs );
			if ( Outwin->onscreen )
			{
				if ( Outwin->fullscreen == 0 )
				{
					t_sync_attr( Outwin->ftattrs );
					t_sync_character_set();
				}
				else
				{
					set = Outwin->character_set;
					selection = 
					    Outwin->select_character_set[ set ];
					term_se_select_character_set( set,
								selection );
					term_se_lock_shift( set );
				}
			}
		}
		else
		{
			if ( Outwin->onscreen )
				if ( Outwin->fullscreen == 0 )
					t_sync_attr( Outwin->ftattrs );
		}
	}
	/******************************************************************
	* Adjust window buffer.
	******************************************************************/
	Outwin->xenl = 0;
	Outwin->real_xenl = 0;
	Outwin->row = 0;
	Outwin->col = 0;
	d_sync_line_attribute_current();		/* clear screen home */
	b_cursor_home();			/* pan buffer to top */
	w_attr_mask = oF_clear_screen_w_attr[ clear_screen_no ];
	blank_char_w_attr = (FTCHAR) blank_char
			  | ( Outwin->ftattrs & w_attr_mask );
	if (  oF_clear_screen_unprotected[ clear_screen_no ]  
	   && Outwin->write_protect_on )
	{
		FTCHAR	protect_ftattrs;

		protect_ftattrs = oF_clear_screen_unprotected[ clear_screen_no];
		/**********************************************************
		* Clear unprotected with write protect on.
		* Find an unprotected spot for cursor.
		* Clear is with or without attributes and possibly to
		* a specified character.
		**********************************************************/
		d_cursor_first_unprotected();
		if ( oF_clear_screen_w_attr[ clear_screen_no ] )
		{
			d_blankwin_unprotected( protect_ftattrs,
						blank_char_w_attr );
			fct_clear_all_common( term_clear_screen, 
						clear_screen_no,
						out_term_clr_eol_w_attr, 
						1, blank_char );
		}
		else
		{
			d_blankwin_unprotected( protect_ftattrs,
						(FTCHAR) blank_char );
			fct_clear_all_common( term_clear_screen, 
						clear_screen_no,
						out_term_clr_eol, 
						1, blank_char );
		}
	}
	else
	{
		/**********************************************************
		* No write protect.
		* Clear is with or without attribute and possibly to a
		* specific character.
		**********************************************************/
		if ( oF_clear_screen_w_attr[ clear_screen_no ] )
		{
			d_blankwin_w_attr( blank_char_w_attr );
			fct_clear_all_common( term_clear_screen, 
						clear_screen_no,
						out_term_clr_eol_w_attr, 
						0, blank_char );
		}
		else
		{
			d_blankwin();
			fct_clear_all_common( term_clear_screen, 
						clear_screen_no,
						out_term_clr_eol, 
						0, blank_char );
		}
	}
}
/**************************************************************************
* win_se_clear_screen
*	An operation has occurred that has the side effect of clearing
*	the current output window.
**************************************************************************/
win_se_clear_screen()
{
	Outwin->xenl = 0;
	Outwin->real_xenl = 0;
	Outwin->row = 0;
	Outwin->col = 0;
	b_cursor_home();			/* pan buffer to top */
	d_blankwin();
	d_sync_line_attribute_current();		/* clear screen home */
}
/**************************************************************************
* fct_clear_all
*	ACTION module for 'clear_all'.
**************************************************************************/
fct_clear_all()
{
	int	term_clear_all();
	int	out_term_clr_eol();

	d_blankwin();
	fct_clear_all_common( term_clear_all, 0, out_term_clr_eol, 0, ' ' );
}
/**************************************************************************
* fct_clear_all_w_attr
*	ACTION module for 'clear_all_w_attr'.
**************************************************************************/
fct_clear_all_w_attr()
{
	int	term_clear_all_w_attr();
	int	out_term_clr_eol_w_attr();
	FTCHAR  blank_char_w_attr;

	blank_char_w_attr = (FTCHAR) ' ' |
		( Outwin->ftattrs & oT_clear_all_w_attr_mask );
	d_blankwin_w_attr( blank_char_w_attr );
	fct_clear_all_common( term_clear_all_w_attr, 0,
			      out_term_clr_eol_w_attr, 0, ' ' );
}
/**************************************************************************
* fct_clear_all_chars_erasable
*	ACTION module for 'clear_all_chars_erasable'.
**************************************************************************/
fct_clear_all_chars_erasable()
{
	int	term_clear_all_chars_erasable();
	int	out_term_clr_eol_chars_erasable();

	d_blankwin_chars_erasable( oT_clear_all_chars_erasable_attr );
	fct_clear_all_common( term_clear_all_chars_erasable, 0, 
				out_term_clr_eol_chars_erasable, 0, ' ' );
}
/**************************************************************************
* fct_clear_all_common
*	Common routine for various flavors of "clear all".  It is assumed
*	that the window buffer has already been handled.
*	func_term_clear_all
*		function to do the appropriate clear all on the terminal.
*	clear_no
*		possible index necessary for func_term_clear_all to select
*		the appropriate clear.
*	func_term_clr_eol
*		a function compatible with func_term_clear_all that does a
*		clear end of line - for use on split screen.
*	unprotected
*		1 = this is a clear unprotected characters. Split screen
*		    is done with a refresh.
*		0 = NOT clear unprotected
*	blank_char
*		a possible argument to func_term_clear_all if the screen is
*		being cleared to a character rather than to a blank.
**************************************************************************/
fct_clear_all_common( func_term_clear_all, clear_no, func_term_clr_eol,
						unprotected, blank_char )
	int	(*func_term_clear_all)();
	int	clear_no;
	int	(*func_term_clr_eol)();
	int	unprotected;
	char	blank_char;
{
	if ( Outwin->onscreen )
	{
		if ( Outwin->fullscreen )
			(*func_term_clear_all)( clear_no, blank_char );
		else 
		{
			if ( unprotected ) /* ??? */
			{
				modes_off_redo_screen();
				split_refresh();
				s_cursor();
				modes_on_redo_screen();
			}
			else
				s_clear_all_common( func_term_clr_eol );
		}
	}
}
/**************************************************************************
* s_clear_all_common
*	split screen support for fct_clear_all_common
**************************************************************************/
s_clear_all_common( func_term_clr_eol )
	int	(*func_term_clr_eol)();
{
	int	row;

				/* ??? use linefeed ??? */
	for ( row = Outwin->win_top_row;
	      row <= Outwin->win_bot_row;
	      row++ )
	{
		term_pos( row, 0 );
		(*func_term_clr_eol)();
		term_line_attribute( Outwin->line_attribute[ row ] );
	}
	s_cursor();
}
/**************************************************************************
* dec_clr_eol
*	DECODE module for 'clr_eol'.
**************************************************************************/
dec_clr_eol()
{
	fct_clr_eol();
}
/**************************************************************************
* inst_clr_eol
*	INSTALL module for 'clr_eol'.
**************************************************************************/
inst_clr_eol( str )
	char	*str;
{
	dec_install( "clr_eol", (UNCHAR *) str, dec_clr_eol, 0, 0,
	(char *) 0 );
}
/**************************************************************************
* dec_clr_eol_w_attr
*	DECODE module for 'clr_eol_w_attr'.
**************************************************************************/
dec_clr_eol_w_attr()
{
	fct_clr_eol_w_attr();
}
/**************************************************************************
* inst_clr_eol_w_attr
*	INSTALL module for 'clr_eol_w_attr'.
**************************************************************************/
inst_clr_eol_w_attr( str )
	char	*str;
{
	dec_install( "clr_eol_w_attr", (UNCHAR *) str, dec_clr_eol_w_attr,
		0, 0,
		(char *) 0 );
}
/**************************************************************************
* dec_clr_eol_chars
*	DECODE module for 'clr_eol_chars'.
**************************************************************************/
dec_clr_eol_chars()
{
	fct_clr_eol_chars();
}
/**************************************************************************
* inst_clr_eol_chars
*	INSTALL module for 'clr_eol_chars'.
**************************************************************************/
inst_clr_eol_chars( str )
	char	*str;
{
	dec_install( "clr_eol_chars", (UNCHAR *) str, dec_clr_eol_chars,
		0, 0,
		(char *) 0 );
}
/**************************************************************************
* dec_set_attr_eol
*	DECODE module for 'set_attr_eol'.
**************************************************************************/
dec_set_attr_eol()
{
	fct_set_attr_eol();
}
/**************************************************************************
* inst_set_attr_eol
*	INSTALL module for 'set_attr_eol'.
**************************************************************************/
inst_set_attr_eol( str )
	char	*str;
{
	dec_install( "set_attr_eol", (UNCHAR *) str, dec_set_attr_eol,
		0, 0,
		(char *) 0 );
}
/**************************************************************************
* dec_clr_eol_unprotected
*	DECODE module for 'clr_eol_unprotected'.
**************************************************************************/
dec_clr_eol_unprotected()
{
	fct_clr_eol_unprotected();
}
/**************************************************************************
* inst_clr_eol_unprotected
*	INSTALL module for 'clr_eol_unprotected'.
**************************************************************************/
inst_clr_eol_unprotected( str )
	char	*str;
{
	dec_install( "clr_eol_unprotected", (UNCHAR *) str,
		dec_clr_eol_unprotected,
		0, 0,
		(char *) 0 );
}
/**************************************************************************
* dec_clr_eol_unprotected_w_attr
*	DECODE module for 'clr_eol_unprotected_w_attr'.
**************************************************************************/
dec_clr_eol_unprotected_w_attr()
{
	fct_clr_eol_unprotected_w_attr();
}
/**************************************************************************
* inst_clr_eol_unprotected_w_attr
*	INSTALL module for 'clr_eol_unprotected_w_attr'.
**************************************************************************/
inst_clr_eol_unprotected_w_attr( str )
	char	*str;
{
	dec_install( "clr_eol_unprotected_w_attr", (UNCHAR *) str,
		dec_clr_eol_unprotected_w_attr,
		0, 0,
		(char *) 0 );
}
/**************************************************************************
* dec_clr_fld_unprotected
*	DECODE module for 'clr_fld_unprotected'.
**************************************************************************/
dec_clr_fld_unprotected()
{
	fct_clr_fld_unprotected();
}
/**************************************************************************
* inst_clr_fld_unprotected
*	INSTALL module for 'clr_fld_unprotected'.
**************************************************************************/
inst_clr_fld_unprotected( str )
	char	*str;
{
	dec_install( "clr_fld_unprotected", (UNCHAR *) str,
		dec_clr_fld_unprotected,
		0, 0,
		(char *) 0 );
}
/**************************************************************************
* dec_clr_fld_unprotected_w_attr
*	DECODE module for 'clr_fld_unprotected_w_attr'.
**************************************************************************/
dec_clr_fld_unprotected_w_attr()
{
	fct_clr_fld_unprotected_w_attr();
}
/**************************************************************************
* inst_clr_fld_unprotected_w_attr
*	INSTALL module for 'clr_fld_unprotected_w_attr'.
**************************************************************************/
inst_clr_fld_unprotected_w_attr( str )
	char	*str;
{
	dec_install( "clr_fld_unprotected_w_attr", (UNCHAR *) str,
		dec_clr_fld_unprotected_w_attr,
		0, 0,
		(char *) 0 );
}
/**************************************************************************
* dec_clr_bol
*	DECODE module for 'clr_bol'.
**************************************************************************/
dec_clr_bol()
{
	fct_clr_bol();
}
/**************************************************************************
* inst_clr_bol
*	INSTALL module for 'clr_bol'.
**************************************************************************/
inst_clr_bol( str )
	char	*str;
{
	dec_install( "clr_bol", (UNCHAR *) str, dec_clr_bol, 0, 0,
	(char *) 0 );
}
/**************************************************************************
* dec_clr_bol_w_attr
*	DECODE module for 'clr_bol_w_attr'.
**************************************************************************/
dec_clr_bol_w_attr()
{
	fct_clr_bol_w_attr();
}
/**************************************************************************
* inst_clr_bol_w_attr
*	INSTALL module for 'clr_bol_w_attr'.
**************************************************************************/
inst_clr_bol_w_attr( str )
	char	*str;
{
	dec_install( "clr_bol_w_attr", (UNCHAR *) str, dec_clr_bol_w_attr,
		0, 0,
		(char *) 0 );
}
/**************************************************************************
* dec_clr_bol_chars
*	DECODE module for 'clr_bol_chars'.
**************************************************************************/
dec_clr_bol_chars()
{
	fct_clr_bol_chars();
}
/**************************************************************************
* inst_clr_bol_chars
*	INSTALL module for 'clr_bol_chars'.
**************************************************************************/
inst_clr_bol_chars( str )
	char	*str;
{
	dec_install( "clr_bol_chars", (UNCHAR *) str, dec_clr_bol_chars,
		0, 0,
		(char *) 0 );
}
/**************************************************************************
* dec_set_attr_bol
*	DECODE module for 'set_attr_bol'.
**************************************************************************/
dec_set_attr_bol()
{
	fct_set_attr_bol();
}
/**************************************************************************
* inst_set_attr_bol
*	INSTALL module for 'set_attr_bol'.
**************************************************************************/
inst_set_attr_bol( str )
	char	*str;
{
	dec_install( "set_attr_bol", (UNCHAR *) str, dec_set_attr_bol,
		0, 0,
		(char *) 0 );
}
/**************************************************************************
* dec_clear_line
*	DECODE module for 'clear_line'.
**************************************************************************/
dec_clear_line()
{
	fct_clear_line();
}
/**************************************************************************
* inst_clear_line
*	INSTALL module for 'clear_line'.
**************************************************************************/
inst_clear_line( str )
	char	*str;
{
	dec_install( "clear_line", (UNCHAR *) str, dec_clear_line, 0, 0,
	(char *) 0 );
}
/**************************************************************************
* dec_clear_line_w_attr
*	DECODE module for 'clear_line_w_attr'.
**************************************************************************/
dec_clear_line_w_attr()
{
	fct_clear_line_w_attr();
}
/**************************************************************************
* inst_clear_line_w_attr
*	INSTALL module for 'clear_line_w_attr'.
**************************************************************************/
inst_clear_line_w_attr( str )
	char	*str;
{
	dec_install( "clear_line_w_attr", (UNCHAR *) str, dec_clear_line_w_attr,
		0, 0,
		(char *) 0 );
}
/**************************************************************************
* dec_clear_line_chars
*	DECODE module for 'clear_line_chars'.
**************************************************************************/
dec_clear_line_chars()
{
	fct_clear_line_chars();
}
/**************************************************************************
* inst_clear_line_chars
*	INSTALL module for 'clear_line_chars'.
**************************************************************************/
inst_clear_line_chars( str )
	char	*str;
{
	dec_install( "clear_line_chars", (UNCHAR *) str, dec_clear_line_chars,
		0, 0,
		(char *) 0 );
}
/**************************************************************************
* dec_set_attr_line
*	DECODE module for 'set_attr_line'.
**************************************************************************/
dec_set_attr_line()
{
	fct_set_attr_line();
}
/**************************************************************************
* inst_set_attr_line
*	INSTALL module for 'set_attr_line'.
**************************************************************************/
inst_set_attr_line( str )
	char	*str;
{
	dec_install( "set_attr_line", (UNCHAR *) str, dec_set_attr_line,
		0, 0,
		(char *) 0 );
}
/**************************************************************************
* dec_clr_eos
*	DECODE module for 'clr_eos'.
**************************************************************************/
dec_clr_eos()
{
	fct_clr_eos();
}
/**************************************************************************
* inst_clr_eos
*	INSTALL module for 'clr_eos'.
**************************************************************************/
inst_clr_eos( str )
	char	*str;
{
	dec_install( "clr_eos", (UNCHAR *) str, dec_clr_eos, 0, 0,
	(char *) 0 );
}
/**************************************************************************
* dec_clr_eos_w_attr
*	DECODE module for 'clr_eos_w_attr'.
**************************************************************************/
dec_clr_eos_w_attr()
{
	fct_clr_eos_w_attr();
}
/**************************************************************************
* inst_clr_eos_w_attr
*	INSTALL module for 'clr_eos_w_attr'.
**************************************************************************/
inst_clr_eos_w_attr( str )
	char	*str;
{
	dec_install( "clr_eos_w_attr", (UNCHAR *) str, dec_clr_eos_w_attr, 
		0, 0,
		(char *) 0 );
}
/**************************************************************************
* dec_clr_eos_chars
*	DECODE module for 'clr_eos_chars'.
**************************************************************************/
dec_clr_eos_chars()
{
	fct_clr_eos_chars();
}
/**************************************************************************
* inst_clr_eos_chars
*	INSTALL module for 'clr_eos_chars'.
**************************************************************************/
inst_clr_eos_chars( str )
	char	*str;
{
	dec_install( "clr_eos_chars", (UNCHAR *) str, dec_clr_eos_chars, 
		0, 0,
		(char *) 0 );
}
/**************************************************************************
* dec_set_attr_eos
*	DECODE module for 'set_attr_eos'.
**************************************************************************/
dec_set_attr_eos()
{
	fct_set_attr_eos();
}
/**************************************************************************
* inst_set_attr_eos
*	INSTALL module for 'set_attr_eos'.
**************************************************************************/
inst_set_attr_eos( str )
	char	*str;
{
	dec_install( "set_attr_eos", (UNCHAR *) str, dec_set_attr_eos, 
		0, 0,
		(char *) 0 );
}
/**************************************************************************
* dec_clr_eos_chars_erasable
*	DECODE module for 'clr_eos_chars_erasable'.
**************************************************************************/
dec_clr_eos_chars_erasable()
{
	fct_clr_eos_chars_erasable();
}
/**************************************************************************
* inst_clr_eos_chars_erasable
*	INSTALL module for 'clr_eos_chars_erasable'.
**************************************************************************/
inst_clr_eos_chars_erasable( str )
	char	*str;
{
	dec_install( "clr_eos_chars_erasable", (UNCHAR *) str, 
					dec_clr_eos_chars_erasable, 0, 0,
					(char *) 0 );
}
/**************************************************************************
* dec_clr_eos_unprotected
*	DECODE module for 'clr_eos_unprotected'.
**************************************************************************/
dec_clr_eos_unprotected()
{
	fct_clr_eos_unprotected();
}
/**************************************************************************
* inst_clr_eos_unprotected
*	INSTALL module for 'clr_eos_unprotected'.
**************************************************************************/
inst_clr_eos_unprotected( str )
	char	*str;
{
	dec_install( "clr_eos_unprotected", (UNCHAR *) str, 
					dec_clr_eos_unprotected, 0, 0,
					(char *) 0 );
}
/**************************************************************************
* dec_clr_eos_unprotected_w_attr
*	DECODE module for 'clr_eos_unprotected_w_attr'.
**************************************************************************/
dec_clr_eos_unprotected_w_attr()
{
	fct_clr_eos_unprotected_w_attr();
}
/**************************************************************************
* inst_clr_eos_unprotected_w_attr
*	INSTALL module for 'clr_eos_unprotected_w_attr'.
**************************************************************************/
inst_clr_eos_unprotected_w_attr( str )
	char	*str;
{
	dec_install( "clr_eos_unprotected_w_attr", (UNCHAR *) str, 
					dec_clr_eos_unprotected_w_attr, 0, 0,
					(char *) 0 );
}
/**************************************************************************
* fct_clr_bos
*	ACTION module for 'clr_bos'.
**************************************************************************/
fct_clr_bos()
{
	int	d_clr_bos();
	int	term_clr_bos();
	int	out_term_clr_bol();
	int	out_term_clr_eol();

	fct_clr_bos_common( d_clr_bos, (FTCHAR) 0,
			    term_clr_bos, out_term_clr_bol,
			    out_term_clr_eol );
}
/**************************************************************************
* fct_clr_bos_chars_erasable
*	ACTION module for 'clr_bos_chars_erasable'.
**************************************************************************/
fct_clr_bos_chars_erasable()
{
	int	d_clr_bos_chars_erasable();
	int	term_clr_bos_chars_erasable();
	int	out_term_clr_bol_chars_erasable();
	int	out_term_clr_eol_chars_erasable();

	fct_clr_bos_common( d_clr_bos_chars_erasable, 
			    oT_clr_bos_chars_erasable_attr,
			    term_clr_bos_chars_erasable, 
			    out_term_clr_bol_chars_erasable,
			    out_term_clr_eol_chars_erasable );
}
/**************************************************************************
* fct_clr_bos_common
*	Common routine for various flavors of "clear to beginning of screen".
*	func_d_clr_bos
*		function to do the appropriate clear to beginning of screen on
*		the window buffer.
*	func_d_clr_bos_parm
*		possible attribute parameter to func_d_clr_bos if its
*		clear is attribute dependent.
*	func_term_clr_bos
*		function to do the appropriate clear to beginning of screen on
*		the terminal.
*	func_term_clr_bol
*		a function compatible with func_term_clr_bos that does a
*		clear beginning of line - for use on split screen.
*	func_term_clr_eol
*		a function compatible with func_term_clr_bos that does a
*		clear end of line - for use on split screen.
**************************************************************************/
fct_clr_bos_common( func_d_clr_bos, func_d_clr_bos_parm,
				func_term_clr_bos, func_term_clr_bol,
				func_term_clr_eol )
	int	(*func_d_clr_bos)();
	FTCHAR	func_d_clr_bos_parm;
	int	(*func_term_clr_bos)();
	int	(*func_term_clr_bol)();
	int	(*func_term_clr_eol)();
{
	int	affrow;

	(*func_d_clr_bos)( Outwin->row, Outwin->col, func_d_clr_bos_parm );
	if ( Outwin->onscreen )
	{
		if ( Outwin->fullscreen )
		{
			affrow = Outwin->row;
			(*func_term_clr_bos)( affrow );
		}
		else
			s_clr_bos_common( func_term_clr_bol,
					  func_term_clr_eol );
	}
}
/**************************************************************************
* s_clr_bos_common
*	split screen support for fct_clr_bos_common
**************************************************************************/
				/* screen clr to top for onscreen split */
s_clr_bos_common( func_term_clr_bol, func_term_clr_eol )
	int	(*func_term_clr_bol)();
	int	(*func_term_clr_eol)();
{
	int	row;
		/* ??? use linefeed ??? */

					/* possible partial first line */
	(*func_term_clr_bol)();
					/* clear rest of visible lines */
	for ( row = Outwin->win_top_row
		    + Outwin->row - 1
		    - Outwin->buff_top_row;
	      row >= Outwin->win_top_row;
	      row-- )
	{
		term_pos( row, 0 );
		(*func_term_clr_eol)();
		term_line_attribute( Outwin->line_attribute[ row ] );
	}
	s_cursor();
}
/**************************************************************************
* dec_clr_bos
*	DECODE module for 'clr_bos'.
**************************************************************************/
dec_clr_bos()
{
	fct_clr_bos();
}
/**************************************************************************
* inst_clr_bos
*	INSTALL module for 'clr_bos'.
**************************************************************************/
inst_clr_bos( str )
	char	*str;
{
	dec_install( "clr_bos", (UNCHAR *) str, dec_clr_bos, 0, 0,
	(char *) 0 );
}
/**************************************************************************
* dec_clr_bos_chars_erasable
*	DECODE module for 'clr_bos_chars_erasable'.
**************************************************************************/
dec_clr_bos_chars_erasable()
{
	fct_clr_bos_chars_erasable();
}
/**************************************************************************
* inst_clr_bos_chars_erasable
*	INSTALL module for 'clr_bos_chars_erasable'.
**************************************************************************/
inst_clr_bos_chars_erasable( str )
	char	*str;
{
	dec_install( "clr_bos_chars_erasable", (UNCHAR *) str, 
					dec_clr_bos_chars_erasable, 0, 0,
					(char *) 0 );
}
/***************************************************************************/
/**************************************************************************
* term_erase_chars
*	TERMINAL OUTPUT module for 'erase_chars'.
**************************************************************************/
char	*T_erase_chars = (char *) 0;
term_erase_chars( cols )
	int	cols;
{
	char	*p;
	int	parm[ 2 ];			/* 1 used - must be >= 2 - %i */
	char	*string_parm[ 1 ];		/* not used */
	char	*my_tparm();

	if ( T_erase_chars != (char *) 0 )
	{
		parm[ 0 ] = cols;
		p = my_tparm( T_erase_chars, parm, string_parm, -1 );
		my_tputs( p, cols );
		return;
	}
}
/**************************************************************************
* fct_erase_chars
*	ACTION module for 'erase_chars'.
**************************************************************************/
fct_erase_chars( count )
	int	count;
{
	int	cols;

	cols = count;
	if ( cols < 1 )
		cols = 1;
	else if ( (Outwin->col + cols - 1) > Outwin->col_right_line )
		cols = Outwin->col_right_line - Outwin->col + 1;
	d_erase_chars( Outwin->row, Outwin->col, cols );
	if ( Outwin->onscreen )
		term_erase_chars( cols );
}
/**************************************************************************
* dec_erase_chars
*	DECODE module for 'erase_chars'.
**************************************************************************/
/*ARGSUSED*/
dec_erase_chars( func_parm, parm_ptr, parms_valid, parm )
	int	func_parm;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
{
	int	count;

	if ( parms_valid & 1 )
		count = parm[ 0 ];
	else
		count = 0;
	fct_erase_chars( count );
}
/**************************************************************************
* inst_erase_chars
*	INSTALL module for 'erase_chars'.
**************************************************************************/
inst_erase_chars( str )
	char	*str;
{
	dec_install( "erase_chars", (UNCHAR *) str, 
					dec_erase_chars, 0, CURSOR_OPTION,
					(char *) 0 );
}
/***************************************************************************/
/**************************************************************************
* extra_clear
*	TERMINAL DESCRIPTION PARSER module for 'clear...' and 'clr...'.
**************************************************************************/
char *dec_encode();
extra_clear( buff, string, attr_on_string, attr_off_string ) 
	char	*buff;
	char	*string;
	char	*attr_on_string;
	char	*attr_off_string;
{
	FTCHAR	attribute_encode();
	char	*break_dash_string();
	char	*p;

	if ( strcmp( buff, "clear_screen" ) == 0 )
	{
		if ( xT_clear_screen_no < MAX_CLEAR_SCREEN )
		{
			ex_clear_screen_common( xT_clear_screen_no, 0, 0, 
				   string, attr_on_string, attr_off_string );
			xT_clear_screen_no++;
		}
		else
			printf( "Too many clear_screen\n" );
	}
	else if ( strcmp( buff, "out_clear_screen" ) == 0 )
	{
		xT_out_clear_screen= dec_encode( string );
	}
	else if ( strcmp( buff, "clear_screen_unprotected" ) == 0 )
	{
		if ( xT_clear_screen_no < MAX_CLEAR_SCREEN )
		{
			ex_clear_screen_common( xT_clear_screen_no, 1, 0, 
				   string, attr_on_string, attr_off_string );
			xT_clear_screen_no++;
		}
		else
			printf( "Too many clear_screen_unprotected\n" );
	}
	else if ( strcmp( buff, "clear_screen_w_attr" ) == 0 )
	{
		if ( xT_clear_screen_no < MAX_CLEAR_SCREEN )
		{
			ex_clear_screen_common( xT_clear_screen_no, 0, 1, 
				   string, attr_on_string, attr_off_string );
			xT_clear_screen_no++;
		}
		else
			printf( "Too many clear_screen_w_attr\n" );
	}
	else if ( strcmp( buff, "clear_screen_unprotected_w_attr" ) == 0 )
	{
		if ( xT_clear_screen_no < MAX_CLEAR_SCREEN )
		{
			ex_clear_screen_common( xT_clear_screen_no, 1, 1, 
				   string, attr_on_string, attr_off_string );
			xT_clear_screen_no++;
		}
		else
			printf( "Too many clear_screen_unprotected_w_attr\n" );
	}
	else if ( strcmp( buff, "clear_all" ) == 0 )
	{
		xT_clear_all = dec_encode( string );
		inst_clear_all( xT_clear_all );
	}
	else if ( strcmp( buff, "clear_all_w_attr" ) == 0 )
	{
		xT_clear_all_w_attr = dec_encode( string );
		inst_clear_all_w_attr( xT_clear_all_w_attr );
		p = break_dash_string( attr_off_string );
		xT_clear_all_w_attr_mask = attribute_encode( p );
		if ( xT_clear_all_w_attr_mask == 0 )
			xT_clear_all_w_attr_mask = ATTR_PROTECT;
	}
	else if ( strcmp( buff, "clear_all_chars_erasable" ) == 0 )
	{
		xT_clear_all_chars_erasable_attr =
					attribute_encode( attr_on_string );
		if ( xT_clear_all_chars_erasable_attr == 0)
		{
			printf( 
			"Attribute missing on clear_all_chars_erasable\n" );
		}
		else
		{
			xT_clear_all_chars_erasable = dec_encode( string );
			inst_clear_all_chars_erasable( 
						xT_clear_all_chars_erasable );
		}
	}
	else if ( strcmp( buff, "clr_eol" ) == 0 )
	{
		xT_clr_eol = dec_encode( string );
		inst_clr_eol( xT_clr_eol );
	}
	else if ( strcmp( buff, "out_clr_memory_below" ) == 0 )
	{
		xF_memory_below = 1;
		xT_out_clr_memory_below = dec_encode( string );
	}
	else if ( strcmp( buff, "out_clr_eol" ) == 0 )
	{
		xT_out_clr_eol = dec_encode( string );
	}
	else if ( strcmp( buff, "clr_eol_w_attr" ) == 0 )
	{
		xT_clr_eol_w_attr = dec_encode( string );
		inst_clr_eol_w_attr( xT_clr_eol_w_attr );
		p = break_dash_string( attr_off_string );
		xT_clr_eol_w_attr_mask = attribute_encode( p );
		if ( xT_clr_eol_w_attr_mask == 0 )
			xT_clr_eol_w_attr_mask = ATTR_PROTECT;
	}
	else if ( strcmp( buff, "out_clr_eol_w_attr" ) == 0 )
	{
		xT_out_clr_eol_w_attr = dec_encode( string );
	}
	else if ( strcmp( buff, "clr_eol_chars" ) == 0 )
	{
		xT_clr_eol_chars = dec_encode( string );
		inst_clr_eol_chars( xT_clr_eol_chars );
	}
	else if ( strcmp( buff, "set_attr_eol" ) == 0 )
	{
		T_set_attr_eol = dec_encode( string );
		inst_set_attr_eol( T_set_attr_eol );
	}
	else if ( strcmp( buff, "out_clr_eol_chars_erasable" ) == 0 )
	{
		xT_out_clr_eol_chars_erasable = dec_encode( string );
	}
	else if ( strcmp( buff, "clr_eol_unprotected" ) == 0 )
	{
		xT_clr_eol_unprotected = dec_encode( string );
		inst_clr_eol_unprotected( xT_clr_eol_unprotected );
		xT_clr_eol_unprotected_attr =
					attribute_encode( attr_on_string );
		if ( xT_clr_eol_unprotected_attr == 0)
			xT_clr_eol_unprotected_attr = ATTR_PROTECT;
	}
	else if ( strcmp( buff, "out_clr_eol_unprotected" ) == 0 )
	{
		xT_out_clr_eol_unprotected = dec_encode( string );
	}
	else if ( strcmp( buff, "clr_eol_unprotected_w_attr" ) == 0 )
	{
		xT_clr_eol_unprotected_w_attr = dec_encode( string );
		inst_clr_eol_unprotected_w_attr( xT_clr_eol_unprotected_w_attr);
		xT_clr_eol_unprotected_w_attr_attr =
					attribute_encode( attr_on_string );
		if ( xT_clr_eol_unprotected_w_attr_attr == 0)
			xT_clr_eol_unprotected_w_attr_attr = ATTR_PROTECT;
		p = break_dash_string( attr_off_string );
		xT_clr_eol_unprotected_w_attr_mask = attribute_encode( p );
		if ( xT_clr_eol_unprotected_w_attr_mask == 0 )
			xT_clr_eol_unprotected_w_attr_mask = ATTR_PROTECT;
	}
	else if ( strcmp( buff, "out_clr_eol_unprotected_w_attr" ) == 0 )
	{
		xT_out_clr_eol_unprotected_w_attr = dec_encode( string );
	}
	else if ( strcmp( buff, "clr_fld_unprotected" ) == 0 )
	{
		xT_clr_fld_unprotected = dec_encode( string );
		inst_clr_fld_unprotected( xT_clr_fld_unprotected );
		xT_clr_fld_unprotected_attr =
					attribute_encode( attr_on_string );
		if ( xT_clr_fld_unprotected_attr == 0)
			xT_clr_fld_unprotected_attr = ATTR_PROTECT;
	}
	else if ( strcmp( buff, "out_clr_fld_unprotected" ) == 0 )
	{
		xT_out_clr_fld_unprotected = dec_encode( string );
	}
	else if ( strcmp( buff, "clr_fld_unprotected_w_attr" ) == 0 )
	{
		xT_clr_fld_unprotected_w_attr = dec_encode( string );
		inst_clr_fld_unprotected_w_attr( xT_clr_fld_unprotected_w_attr);
		xT_clr_fld_unprotected_w_attr_attr =
					attribute_encode( attr_on_string );
		if ( xT_clr_fld_unprotected_w_attr_attr == 0)
			xT_clr_fld_unprotected_w_attr_attr = ATTR_PROTECT;
		p = break_dash_string( attr_off_string );
		xT_clr_fld_unprotected_w_attr_mask = attribute_encode( p );
		if ( xT_clr_fld_unprotected_w_attr_mask == 0 )
			xT_clr_fld_unprotected_w_attr_mask = ATTR_PROTECT;
	}
	else if ( strcmp( buff, "out_clr_fld_unprotected_w_attr" ) == 0 )
	{
		xT_out_clr_fld_unprotected_w_attr = dec_encode( string );
	}
	else if ( strcmp( buff, "out_clr_bol_chars_erasable" ) == 0 )
	{
		xT_out_clr_bol_chars_erasable = dec_encode( string );
	}
	else if ( strcmp( buff, "clr_bol" ) == 0 )
	{
		xT_clr_bol = dec_encode( string );
		inst_clr_bol( xT_clr_bol );
	}
	else if ( strcmp( buff, "out_clr_bol" ) == 0 )
	{
		xT_out_clr_bol = dec_encode( string );
	}
	else if ( strcmp( buff, "clr_bol_w_attr" ) == 0 )
	{
		xT_clr_bol_w_attr = dec_encode( string );
		inst_clr_bol_w_attr( xT_clr_bol_w_attr );
	}
	else if ( strcmp( buff, "clr_bol_chars" ) == 0 )
	{
		xT_clr_bol_chars = dec_encode( string );
		inst_clr_bol_chars( xT_clr_bol_chars );
	}
	else if ( strcmp( buff, "set_attr_bol" ) == 0 )
	{
		T_set_attr_bol = dec_encode( string );
		inst_set_attr_bol( T_set_attr_bol );
	}
	else if ( strcmp( buff, "clear_line" ) == 0 )
	{
		xT_clear_line = dec_encode( string );
		inst_clear_line( xT_clear_line );
	}
	else if ( strcmp( buff, "out_clear_line" ) == 0 )
	{
		xT_out_clear_line = dec_encode( string );
	}
	else if ( strcmp( buff, "clear_line_w_attr" ) == 0 )
	{
		xT_clear_line_w_attr = dec_encode( string );
		inst_clear_line_w_attr( xT_clear_line_w_attr );
	}
	else if ( strcmp( buff, "clear_line_chars" ) == 0 )
	{
		xT_clear_line_chars = dec_encode( string );
		inst_clear_line_chars( xT_clear_line_chars );
	}
	else if ( strcmp( buff, "set_attr_line" ) == 0 )
	{
		T_set_attr_line = dec_encode( string );
		inst_set_attr_line( T_set_attr_line );
	}
	else if ( strcmp( buff, "clr_eos" ) == 0 )
	{
		xT_clr_eos = dec_encode( string );
		inst_clr_eos( xT_clr_eos );
	}
	else if ( strcmp( buff, "out_clr_eos" ) == 0 )
	{
		xT_out_clr_eos = dec_encode( string );
	}
	else if ( strcmp( buff, "clr_eos_split_screen_with_refresh" ) == 0 )
	{
		xF_clr_eos_split_screen_with_refresh = get_optional_value( 
								string, 1 );
	}
	else if ( strcmp( buff, "clr_eos_w_attr" ) == 0 )
	{
		xT_clr_eos_w_attr = dec_encode( string );
		inst_clr_eos_w_attr( xT_clr_eos_w_attr );
		p = break_dash_string( attr_off_string );
		xT_clr_eos_w_attr_mask = attribute_encode( p );
		if ( xT_clr_eos_w_attr_mask == 0 )
			xT_clr_eos_w_attr_mask = ATTR_PROTECT;
	}
	else if ( strcmp( buff, "clr_eos_chars" ) == 0 )
	{
		xT_clr_eos_chars = dec_encode( string );
		inst_clr_eos_chars( xT_clr_eos_chars );
	}
	else if ( strcmp( buff, "set_attr_eos" ) == 0 )
	{
		T_set_attr_eos = dec_encode( string );
		inst_set_attr_eos( T_set_attr_eos );
	}
	else if ( strcmp( buff, "clr_eos_chars_erasable" ) == 0 )
	{
		xT_clr_eos_chars_erasable_attr =
					attribute_encode( attr_on_string );
		if ( xT_clr_eos_chars_erasable_attr == 0)
		{
			printf( 
			"Attribute missing on clr_eos_chars_erasable\n" );
		}
		else
		{
			xT_clr_eos_chars_erasable = dec_encode( string );
			inst_clr_eos_chars_erasable( 
						xT_clr_eos_chars_erasable );
		}
	}
	else if ( strcmp( buff, "clr_eos_unprotected" ) == 0 )
	{
		xT_clr_eos_unprotected = dec_encode( string );
		inst_clr_eos_unprotected( xT_clr_eos_unprotected );
		xT_clr_eos_unprotected_attr =
					attribute_encode( attr_on_string );
		if ( xT_clr_eos_unprotected_attr == 0 )
			xT_clr_eos_unprotected_attr = ATTR_PROTECT;
	}
	else if ( strcmp( buff, "clr_eos_unprotected_w_attr" ) == 0 )
	{
		xT_clr_eos_unprotected_w_attr = dec_encode( string );
		inst_clr_eos_unprotected_w_attr( xT_clr_eos_unprotected_w_attr);
		xT_clr_eos_unprotected_w_attr_attr =
					attribute_encode( attr_on_string );
		if ( xT_clr_eos_unprotected_w_attr_attr == 0 )
			xT_clr_eos_unprotected_w_attr_attr = ATTR_PROTECT;
		p = break_dash_string( attr_off_string );
		xT_clr_eos_unprotected_w_attr_mask = attribute_encode( p );
		if ( xT_clr_eos_unprotected_w_attr_mask == 0 )
			xT_clr_eos_unprotected_w_attr_mask = ATTR_PROTECT;
	}
	else if ( strcmp( buff, "clr_bos" ) == 0 )
	{
		xT_clr_bos = dec_encode( string );
		inst_clr_bos( xT_clr_bos );
	}
	else if ( strcmp( buff, "clr_bos_chars_erasable" ) == 0 )
	{
		xT_clr_bos_chars_erasable_attr =
					attribute_encode( attr_on_string );
		if ( xT_clr_bos_chars_erasable_attr == 0)
		{
			printf( 
			"Attribute missing on clr_bos_chars_erasable\n" );
		}
		else
		{
			xT_clr_bos_chars_erasable = dec_encode( string );
			inst_clr_bos_chars_erasable( 
						xT_clr_bos_chars_erasable );
		}
	}
	else if ( strcmp( buff, "erase_chars" ) == 0 )
	{
		T_erase_chars = dec_encode( string );
		inst_erase_chars( T_erase_chars );
	}
	else
		return( 0 );
	return( 1 );
}
/**************************************************************************
* ex_clear_screen_common
*	Parse clear screen terminal description lines.
**************************************************************************/
/*ARGSUSED*/
ex_clear_screen_common( clear_screen_no, unprotected, w_attr, 
			string, attr_on_string, attr_off_string ) 
	int	clear_screen_no;
	int	unprotected;
	int	w_attr;
	char	*string;
	char	*attr_on_string;		/* not used */
	char	*attr_off_string;
{
	FTCHAR	attribute_encode();
	char	*p;
	FTCHAR	w_attr_mask;
	char	*break_dash_string();

	p = break_dash_string( attr_off_string );
	xT_clear_screen[ clear_screen_no ] = dec_encode( string );
	inst_clear_screen( xT_clear_screen[ clear_screen_no ], 
			   clear_screen_no );
	xF_clear_screen_ftattrs_off[ clear_screen_no ] = 
					attribute_encode( attr_off_string );
	if ( unprotected )
	{
		FTCHAR	protect_ftattrs;

		protect_ftattrs = attribute_encode( attr_on_string );
		if ( protect_ftattrs == 0 )
			xF_clear_screen_unprotected[ clear_screen_no ] =
								ATTR_PROTECT;
		else
			xF_clear_screen_unprotected[ clear_screen_no ] =
								protect_ftattrs;
	}
	else
		xF_clear_screen_unprotected[ clear_screen_no ] = 0;
	if ( w_attr )
	{
		w_attr_mask = attribute_encode( p );
		if ( w_attr_mask == 0 )
			w_attr_mask = ATTR_PROTECT;
		xF_clear_screen_w_attr[ clear_screen_no ] = w_attr_mask;
	}
	else
		xF_clear_screen_w_attr[ clear_screen_no ] = 0;
						/* use last for internal */
	xT_out_clear_screen= xT_clear_screen[ clear_screen_no ];
}
