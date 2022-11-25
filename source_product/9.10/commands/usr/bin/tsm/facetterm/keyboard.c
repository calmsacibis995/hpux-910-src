/*****************************************************************************
** Copyright (c) 1990        Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: keyboard.c,v 70.1 92/03/09 15:44:54 ssa Exp $ */
/**************************************************************************
* keyboard.c
*	Handle the terminals whose keyboards have several modes.
*	Application keypad mode and cursor key mode on the vt220 etc.
*	Keypad transmit (yes/no) on the HP.
**************************************************************************/
#include "ftproc.h"
#include "ftwindow.h"
#include "features.h"

#include "keyboard_p.h"
int	M_appl_keypad_mode_on = { 0 };
char	*sT_enter_appl_keypad_mode = { (char *) 0 };
char	*sT_exit_appl_keypad_mode = { (char *) 0 };

int	M_cursor_key_mode_on = 0;
char	*sT_enter_cursor_key_mode = { (char *) 0 };
char	*sT_exit_cursor_key_mode = { (char *) 0 };

int	M_keypad_xmit = 0;
char	*sT_keypad_xmit = { (char *) 0 };
char	*sT_keypad_local = { (char *) 0 };

/**************************************************************************
* dec_enter_appl_keypad_mode
*	DECODE module for 'enter_appl_keypad_mode'.
*	Turn application keypad mode on.
**************************************************************************/
dec_enter_appl_keypad_mode()
{
	fct_enter_appl_keypad_mode();
}
/**************************************************************************
* inst_enter_appl_keypad_mode
*	INSTALL module for 'enter_appl_keypad_mode'.
**************************************************************************/
inst_enter_appl_keypad_mode( str )
	char	*str;
{
	dec_install( "enter_appl_keypad_mode", (UNCHAR *) str, 
			dec_enter_appl_keypad_mode, 0, 0,
			(char *) 0 );
}
/**************************************************************************
* dec_exit_appl_keypad_mode
*	DECODE module for 'exit_appl_keypad_mode'.
*	Turn application keypad mode off.
**************************************************************************/
dec_exit_appl_keypad_mode()
{
	fct_exit_appl_keypad_mode();
}
/**************************************************************************
* inst_exit_appl_keypad_mode
*	INSTALL module for 'exit_appl_keypad_mode'.
**************************************************************************/
inst_exit_appl_keypad_mode( str )
	char	*str;
{
	dec_install( "exit_appl_keypad_mode", (UNCHAR *) str, 
			dec_exit_appl_keypad_mode, 0, 0,
			(char *) 0 );
}
/**************************************************************************
* dec_enter_cursor_key_mode
*	DECODE module for 'enter_cursor_key_mode'.
*	Turn cursor key mode on.
**************************************************************************/
dec_enter_cursor_key_mode()
{
	fct_enter_cursor_key_mode();
}
/**************************************************************************
* inst_enter_cursor_key_mode
*	INSTALL module for 'enter_cursor_key_mode'.
**************************************************************************/
inst_enter_cursor_key_mode( str )
	char	*str;
{
	dec_install( "enter_cursor_key_mode", (UNCHAR *) str, 
			dec_enter_cursor_key_mode, 0, 0,
			(char *) 0 );
}
/**************************************************************************
* dec_exit_cursor_key_mode
*	DECODE module for 'exit_cursor_key_mode'.
*	Turn cursor key mode off.
**************************************************************************/
dec_exit_cursor_key_mode()
{
	fct_exit_cursor_key_mode();
}
/**************************************************************************
* inst_exit_cursor_key_mode
*	INSTALL module for 'exit_cursor_key_mode'.
**************************************************************************/
inst_exit_cursor_key_mode( str )
	char	*str;
{
	dec_install( "exit_cursor_key_mode", (UNCHAR *) str, 
			dec_exit_cursor_key_mode, 0, 0,
			(char *) 0 );
}
/**************************************************************************
* dec_keypad_xmit
*	DECODE module for 'keypad_xmit'.
*	Turn keypad transmit on.
**************************************************************************/
dec_keypad_xmit()
{
	fct_keypad_xmit();
}
/**************************************************************************
* inst_keypad_xmit
*	INSTALL module for 'keypad_xmit'.
**************************************************************************/
inst_keypad_xmit( str )
	char	*str;
{
	dec_install( "keypad_xmit", (UNCHAR *) str, 
			dec_keypad_xmit, 0, 0,
			(char *) 0 );
}
/**************************************************************************
* dec_keypad_local
*	DECODE module for 'keypad_local'.
*	Turm keypad transmit off.
**************************************************************************/
dec_keypad_local()
{
	fct_keypad_local();
}
/**************************************************************************
* inst_keypad_local
*	INSTALL module for 'keypad_local'.
**************************************************************************/
inst_keypad_local( str )
	char	*str;
{
	dec_install( "keypad_local", (UNCHAR *) str, 
			dec_keypad_local, 0, 0,
			(char *) 0 );
}
/**************************************************************************
* fct_enter_appl_keypad_mode
*	ACTION module for 'enter_appl_keypad_mode'.
**************************************************************************/
fct_enter_appl_keypad_mode()
{
	Outwin->real_xenl = 0;
	Outwin->appl_keypad_mode_on = 1;
	if ( outwin_is_curwin() )
		term_enter_appl_keypad_mode();
}
/**************************************************************************
* fct_exit_appl_keypad_mode
*	ACTION module for 'exit_appl_keypad_mode'.
**************************************************************************/
fct_exit_appl_keypad_mode()
{
	Outwin->real_xenl = 0;
	Outwin->appl_keypad_mode_on = 0;
	if ( outwin_is_curwin() )
		term_exit_appl_keypad_mode();
}
/**************************************************************************
* win_se_appl_keypad_mode
*	An event has occurred that had the side effect of setting the
*	application keypad mode of the terminal to the state "appl_keypad_mode".
**************************************************************************/
win_se_appl_keypad_mode( appl_keypad_mode )
	int	appl_keypad_mode;
{
	Outwin->appl_keypad_mode_on = appl_keypad_mode;
}
/**************************************************************************
* fct_enter_cursor_key_mode
*	ACTION module for 'enter_cursor_key_mode'.
**************************************************************************/
fct_enter_cursor_key_mode()
{
	Outwin->real_xenl = 0;
	Outwin->cursor_key_mode_on = 1;
	if ( outwin_is_curwin() )
		term_enter_cursor_key_mode();
}
/**************************************************************************
* fct_exit_cursor_key_mode
*	ACTION module for 'exit_cursor_key_mode'.
**************************************************************************/
fct_exit_cursor_key_mode()
{
	Outwin->real_xenl = 0;
	Outwin->cursor_key_mode_on = 0;
	if ( outwin_is_curwin() )
		term_exit_cursor_key_mode();
}
/**************************************************************************
* win_se_cursor_key_mode
*	An event has occurred that had the side effect of setting the
*	cursor key mode of the terminal to the state "cursor_key_mode".
**************************************************************************/
win_se_cursor_key_mode( cursor_key_mode )
	int	cursor_key_mode;
{
	Outwin->cursor_key_mode_on = cursor_key_mode;
}
/**************************************************************************
* fct_keypad_xmit
*	ACTION module for 'keypad_xmit'.
**************************************************************************/
fct_keypad_xmit()
{
	Outwin->real_xenl = 0;
	Outwin->keypad_xmit = 1;
	if ( outwin_is_curwin() )
		term_keypad_xmit();
}
/**************************************************************************
* fct_keypad_local
*	ACTION module for 'keypad_local'.
**************************************************************************/
fct_keypad_local()
{
	Outwin->real_xenl = 0;
	Outwin->keypad_xmit = 0;
	if ( outwin_is_curwin() )
		term_keypad_local();
}
/**************************************************************************
* win_se_keypad_xmit
*	An event has occurred that had the side effect of setting the
*	keypad transmit of the terminal to the state "keypad_xmit".
**************************************************************************/
win_se_keypad_xmit( keypad_xmit )
	int	keypad_xmit;
{
	Outwin->keypad_xmit = keypad_xmit;
}
/**************************************************************************
* t_sync_appl_keypad_mode
*	Syncronize the terminal, if necessary, to the application keypad
*	mode state "appl_keypad_mode_on".
**************************************************************************/
t_sync_appl_keypad_mode( appl_keypad_mode_on )
	int	appl_keypad_mode_on;
{

	if ( appl_keypad_mode_on != M_appl_keypad_mode_on )
	{
		if ( appl_keypad_mode_on )
			term_enter_appl_keypad_mode();
		else
			term_exit_appl_keypad_mode();
	}
}
/**************************************************************************
* t_sync_keypad_xmit
*	Syncronize the terminal, if necessary, to the keypad transmit
*	state "keypad_xmit".
**************************************************************************/
t_sync_keypad_xmit( keypad_xmit )
	int	keypad_xmit;
{

	if ( keypad_xmit != M_keypad_xmit )
	{
		if ( keypad_xmit )
			term_keypad_xmit();
		else
			term_keypad_local();
	}
}
/**************************************************************************
* t_reload_keypad_xmit
*	Set the terminal to the keypad transmit state of the current
*	output window.
**************************************************************************/
t_reload_keypad_xmit()
{
	if ( Outwin->keypad_xmit )
		term_keypad_xmit();
	else
		term_keypad_local();
}
/**************************************************************************
* term_enter_appl_keypad_mode
*	TERMINAL OUTPUT module for 'enter_appl_keypad_mode'.
**************************************************************************/
term_enter_appl_keypad_mode()
{
	if ( mT_enter_appl_keypad_mode != (char *) 0 )
	{
		term_tputs( mT_enter_appl_keypad_mode );
		M_appl_keypad_mode_on = 1;
	}
}
/**************************************************************************
* term_exit_appl_keypad_mode
*	TERMINAL OUTPUT module for 'exit_appl_keypad_mode'.
**************************************************************************/
term_exit_appl_keypad_mode()
{
	if ( mT_exit_appl_keypad_mode != (char *) 0 )
	{
		term_tputs( mT_exit_appl_keypad_mode );
		M_appl_keypad_mode_on = 0;
	}
}
/**************************************************************************
* term_se_appl_keypad_mode
*	An event has occurred that had the side effect of setting the
*	application keypad mode of the terminal to the state "appl_keypad_mode".
**************************************************************************/
term_se_appl_keypad_mode( appl_keypad_mode )
	int	appl_keypad_mode;
{
	M_appl_keypad_mode_on = appl_keypad_mode;
}
/**************************************************************************
* term_enter_cursor_key_mode
*	TERMINAL OUTPUT module for 'enter_cursor_key_mode'.
**************************************************************************/
term_enter_cursor_key_mode()
{
	if ( mT_enter_cursor_key_mode != (char *) 0 )
	{
		term_tputs( mT_enter_cursor_key_mode );
		M_cursor_key_mode_on = 1;
	}
}
/**************************************************************************
* term_exit_cursor_key_mode
*	TERMINAL OUTPUT module for 'exit_cursor_key_mode'.
**************************************************************************/
term_exit_cursor_key_mode()
{
	if ( mT_exit_cursor_key_mode != (char *) 0 )
	{
		term_tputs( mT_exit_cursor_key_mode );
		M_cursor_key_mode_on = 0;
	}
}
/**************************************************************************
* term_se_cursor_key_mode
*	An event has occurred that had the side effect of setting the
*	cursor key mode of the terminal to the state "cursor_key_mode".
**************************************************************************/
term_se_cursor_key_mode( cursor_key_mode )
	int	cursor_key_mode;
{
	M_cursor_key_mode_on = cursor_key_mode;
}
/**************************************************************************
* term_keypad_xmit
*	TERMINAL OUTPUT module for 'keypad_xmit'.
**************************************************************************/
term_keypad_xmit()
{
	if ( mT_keypad_xmit != (char *) 0 )
	{
		term_tputs( mT_keypad_xmit );
		M_keypad_xmit = 1;
	}
}
/**************************************************************************
* term_keypad_local
*	TERMINAL OUTPUT module for 'keypad_local'.
**************************************************************************/
term_keypad_local()
{
	if ( mT_keypad_local != (char *) 0 )
	{
		term_tputs( mT_keypad_local );
		M_keypad_xmit = 0;
	}
}
/**************************************************************************
* term_se_keypad_xmit
*	An event has occurred that had the side effect of setting the
*	keypad_xmit of the terminal to the state "keypad_xmit".
**************************************************************************/
term_se_keypad_xmit( keypad_xmit )
	int	keypad_xmit;
{
	M_keypad_xmit = keypad_xmit;
}
/**************************************************************************
* extra_keyboard
*	TERMINAL DESCRIPTION PARSER module for:
*		enter_appl_keypad_mode
*		exit_appl_keypad_mode
*		enter_cursor_key_mode
*		exit_cursor_key_mode
*		keypad_xmit
*		keypad_local
**************************************************************************/
/*ARGSUSED*/
extra_keyboard( buffptr, string, attr_on_string, attr_off_string ) 
	char	*buffptr;
	char	*string;
	char	*attr_on_string;		/* not used */
	char	*attr_off_string;		/* not used */
{
	char	*dec_encode();

	if ( strcmp( buffptr, "enter_appl_keypad_mode" ) == 0 )
	{
		xT_enter_appl_keypad_mode = dec_encode( string );
		inst_enter_appl_keypad_mode( 
					xT_enter_appl_keypad_mode );
	}
	else if ( strcmp( buffptr, "exit_appl_keypad_mode" ) == 0 )
	{
		xT_exit_appl_keypad_mode = dec_encode( string );
		inst_exit_appl_keypad_mode( xT_exit_appl_keypad_mode );
	}
	else if ( strcmp( buffptr, "enter_cursor_key_mode" ) == 0 )
	{
		xT_enter_cursor_key_mode = dec_encode( string );
		inst_enter_cursor_key_mode( xT_enter_cursor_key_mode );
	}
	else if ( strcmp( buffptr, "exit_cursor_key_mode" ) == 0 )
	{
		xT_exit_cursor_key_mode = dec_encode( string );
		inst_exit_cursor_key_mode( xT_exit_cursor_key_mode );
	}
	else if ( strcmp( buffptr, "keypad_xmit" ) == 0 )
	{
		xT_keypad_xmit = dec_encode( string );
		inst_keypad_xmit( xT_keypad_xmit );
	}
	else if ( strcmp( buffptr, "keypad_local" ) == 0 )
	{
		xT_keypad_local = dec_encode( string );
		inst_keypad_local( xT_keypad_local );
	}
	else
	{
		return( 0 );		/* no match */
	}
	return( 1 );
}
