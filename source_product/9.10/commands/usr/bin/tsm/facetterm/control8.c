/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: control8.c,v 70.1 92/03/09 15:41:46 ssa Exp $ */
/**************************************************************************
* control8.c
*	Modules to support switching modes of a terminal such as
*	vt220 7bit 8bit or 700/43 scan code yes/no.
**************************************************************************/
#include <stdio.h>
#include "person.h"
#include "ftchar.h"
#include "ftproc.h"
#include "ftwindow.h"
#include "features.h"
int	F_graphics_8_bit = 0;

#include "decode.h"

#include "control8.h"
int	F_input_8_bit = 0;

int	M_terminal_mode = 0;
int	F_terminal_mode_default = 0;

#define TERMINAL_MODE_MAX	5
#define TERMINAL_MODE_NAME_MAX 15

int	T_terminal_mode_no = 0;
char	T_terminal_mode_name[ TERMINAL_MODE_MAX ][ TERMINAL_MODE_NAME_MAX + 1 ]=
{
	"VT100 - 220 ID",
	"VT220 - 8 BIT",
	"VT220 - 7 BIT",
};
int	F_terminal_mode_is_control_8_bit[  TERMINAL_MODE_MAX ] = { 0, 1, 1 };
int	F_terminal_mode_is_graphics_8_bit[ TERMINAL_MODE_MAX ] = { 0, 1, 1 };
int	F_terminal_mode_personality[       TERMINAL_MODE_MAX ] = { 0, 0, 0 };
#ifdef SOFTPC
int	F_terminal_mode_is_scan_code[	   TERMINAL_MODE_MAX ] = { 0 };
#endif

int	Terminal_mode_from_to[ TERMINAL_MODE_MAX ][ TERMINAL_MODE_MAX ] =
{
	{ 0, 1, 2 },
	{ 0, 1, 2 },
	{ 0, 1, 2 }
};

#define TERMINAL_MODE_SELECT_MAX	20
int	T_terminal_mode_select_no = 0;
char	*T_terminal_mode_select[ TERMINAL_MODE_SELECT_MAX ] = { NULL };
int	F_terminal_mode_select_soft_reset[    TERMINAL_MODE_SELECT_MAX ] = 
									{ 0 };
int	F_terminal_mode_select_switch_page_0[ TERMINAL_MODE_SELECT_MAX ] =
									{ 0 };
int	F_terminal_mode_select_clear_screen[  TERMINAL_MODE_SELECT_MAX ] = 
									{ 0 };

int	Terminal_mode_select_from_to[ TERMINAL_MODE_SELECT_MAX ]
				    [ TERMINAL_MODE_MAX ] =
{
	{  0,  0,  0 },
	{  1,  1,  1 },
	{ -1, -1, -1 },
	{ -1, -1, -1 },
	{ -1, -1, -1 },
	{ -1, -1, -1 },
	{ -1, -1, -1 },
	{ -1, -1, -1 },
	{ -1, -1, -1 },
	{ -1, -1, -1 },
	{ -1, -1, -1 },
	{ -1, -1, -1 },
	{ -1, -1, -1 },
	{ -1, -1, -1 },
	{ -1, -1, -1 },
	{ -1, -1, -1 },
	{ -1, -1, -1 },
	{ -1, -1, -1 },
	{ -1, -1, -1 },
	{ -1, -1, -1 },
};
#ifdef SOFTPC
/**************************************************************************
* terminal_mode_is_scan_code_mode
*	Return 1 if the terminal is in scan code mode. 0 Otherwise.
**************************************************************************/
terminal_mode_is_scan_code_mode()
{
	if ( F_terminal_mode_is_scan_code[ M_terminal_mode ] )
		return( 1 );
	return( 0 );
}
#endif

/**************************************************************************
* dec_terminal_mode_select
*	DECODE module for "terminal_mode_select"
**************************************************************************/
/*ARGSUSED*/
dec_terminal_mode_select( terminal_mode_select, parm_ptr )
	int	terminal_mode_select;
	char	*parm_ptr;
{
	fct_terminal_mode( terminal_mode_select );
}
/**************************************************************************
* inst_terminal_mode_select
*	INSTALL module for "terminal_mode_select"
**************************************************************************/
inst_terminal_mode_select( str, terminal_mode_select )
	char	*str;
	int	terminal_mode_select;
{
	dec_install( "terminal_mode_select", (UNCHAR *) str, 
			dec_terminal_mode_select, terminal_mode_select, 0,
			(char *) 0 );
}
#define SOFT_RESET_VT220	1
#define SOFT_RESET_HP		2
#define SOFT_RESET_HPANSI	3
/**************************************************************************
* fct_terminal_mode
*	ACTION modele for "terminal_mode_select"
**************************************************************************/
fct_terminal_mode( terminal_mode_select )
	int	terminal_mode_select;
{
	int	terminal_mode;
	int	clears;
#ifdef SOFTPC
	int	old_terminal_mode_is_scan_code;
#endif

	terminal_mode = Terminal_mode_select_from_to[ terminal_mode_select ]
						    [ Outwin->terminal_mode ];
	if ( terminal_mode < 0 )
	{
		return;
	}
	Outwin->terminal_mode = terminal_mode;
	Outwin->control_8_bit_on = 
		F_terminal_mode_is_control_8_bit[ terminal_mode ];
	Outwin->graphics_8_bit = 
		F_terminal_mode_is_graphics_8_bit[ terminal_mode ];
	Outwin->personality = 
		F_terminal_mode_personality[ terminal_mode ];
	O_pe = Outwin->personality;
#ifdef SOFTPC
	old_terminal_mode_is_scan_code = Outwin->terminal_mode_is_scan_code;
	Outwin->terminal_mode_is_scan_code = 
		F_terminal_mode_is_scan_code[ terminal_mode ];
#endif
	if ( F_terminal_mode_select_clear_screen[ terminal_mode_select ] )
		win_se_clear_screen();
	switch ( F_terminal_mode_select_soft_reset[ terminal_mode_select ] )
	{
	case SOFT_RESET_VT220:
		win_se_soft_reset();
		break;
	case SOFT_RESET_HP:
		win_se_soft_reset_hp();
		break;
	case SOFT_RESET_HPANSI:
		win_se_soft_reset_hpansi();
		break;
	}
	if ( outwin_is_curwin() )
	{
#ifdef SOFTPC
		if (  Outwin->terminal_mode_is_scan_code 
		   == old_terminal_mode_is_scan_code )
		{
			term_terminal_mode( terminal_mode_select, 
					    terminal_mode );
		}
		else
		{
			switch_terminal_mode_receiver_waiting( 
				terminal_mode_select, terminal_mode );
		}
#else
		term_terminal_mode( terminal_mode_select, terminal_mode );
#endif
	}
	if ( Outwin->onscreen )
	{
		if ( Outwin->fullscreen )
			f_cursor();
		else
		{
			clears = F_terminal_mode_select_clear_screen[ 
						terminal_mode_select ];
			if ( clears )
				split_screen_incompatible( clears );
			s_cursor();
		}
	}
}
#ifdef SOFTPC
/**************************************************************************
* switch_terminal_mode_receiver_waiting
*	Switch the terminal to the mode "terminal_mode" using the 
*	terminal mode select command "terminal_mode_select".  This will
*	switch either into or out of scan code mode so it is necessary
*	to have the receiver's attention so that it is not reading the
*	keyboard.
*	Store the relevant information in the Switch_terminal_mode
*	globals and signal the receiver.
*	The actual work is done in chk_terminal_mode_switch which is 
*	called when fsend_get_acks_only receives an acknowledgement
*	to the signal.
**************************************************************************/
int	Switch_terminal_mode = -1;
int	Switch_terminal_mode_tm_select = 0;
int	Switch_terminal_mode_tm = 0;
switch_terminal_mode_receiver_waiting( terminal_mode_select, terminal_mode )
	int	terminal_mode_select;
	int	terminal_mode;
{
	Switch_terminal_mode = Outwin->number;
	Switch_terminal_mode_tm_select = terminal_mode_select;
	Switch_terminal_mode_tm = terminal_mode;
	send_packet_signal();
	while( Switch_terminal_mode >= 0 )
		fsend_get_acks_only();
}
/**************************************************************************
* chk_terminal_mode_switch
*	Check if receiver is responding to a send_packet_signal generated
*	by switch_terminal_mode_receiver_waiting ( above ).
*	If so, perform the terminal mode switch and return 1.
*	Otherwise return 0 to indicate that it was not.
**************************************************************************/
chk_terminal_mode_switch()
{
	if ( Switch_terminal_mode >= 0 )
	{
		if ( outwin_is_curwin() )
		{
			term_terminal_mode( Switch_terminal_mode_tm_select, 
					    Switch_terminal_mode_tm );
		}
		fct_window_mode_ans_curwin();
		Switch_terminal_mode = -1;
		return( 1 );
	}
	return( 0 );
}
#endif
/**************************************************************************
* win_se_soft_reset
*	An event has occurred that has the side effect of doing a 
*	soft reset on the window.
**************************************************************************/
win_se_soft_reset()
{
	win_se_auto_wrap_off();
	win_se_cursor( 1 );
	win_se_insert_mode( 0 );
	win_se_cursor_key_mode( 0 );
	win_se_appl_keypad_mode( 0 );
	win_se_keypad_xmit( 0 );
	win_se_scroll_region_normal();
	win_se_margins_full();
	win_se_character_set_normal();
	win_se_attribute( (FTCHAR) 0 );
	init_save_cursor();
	win_se_origin_mode( 0 );	/* not implemented */
	/******************************************************************
	* The "o" clear group is used in per_window for side effect clears.
	******************************************************************/
	win_se_perwindow_clear( 'o', Outwin->number );
}
/**************************************************************************
* term_se_soft_reset
*	An event has occurred that has the side effect of doing a 
*	soft reset on the terminal.
**************************************************************************/
term_se_soft_reset()
{
	term_se_auto_wrap_off();
	term_se_cursor( 1 );
	term_se_insert_mode( 0 );	/* not remembered */
	term_se_cursor_key_mode( 0 );
	term_se_appl_keypad_mode( 0 );
	term_se_keypad_xmit( 0 );
	term_se_scroll_region_normal();
	term_se_set_margins_normal();
	term_se_character_set_normal();
	term_se_attribute( (FTCHAR) 0 );
	/* save_cursor is software */
	term_se_origin_mode( 0 );	/* not implemented */
	/******************************************************************
	* The "o" clear group is used in per_window for side effect clears.
	******************************************************************/
	term_se_perwindow_clear( 'o' );
}
/**************************************************************************
* win_se_soft_reset_hp
*	An event has occurred that has the side effect of doing a 
*	soft reset on an hp mode window.
**************************************************************************/
win_se_soft_reset_hp()
{
	/* NOT_on_HP   win_se_auto_wrap_off(); */
	win_se_cursor( 1 );
	/* NOT_to_HP   win_se_insert_mode( 0 ); */
	win_se_cursor_key_mode( 0 );
	win_se_appl_keypad_mode( 0 );
	/* NO_AFFECT win_se_keypad_xmit( 0 ); */
	win_se_scroll_region_normal();
	win_se_memory_unlock();
	win_se_margins_full();
	win_se_character_set_normal();
	win_se_attribute( (FTCHAR) 0 );
	init_save_cursor();
	win_se_origin_mode( 0 );	/* not implemented */
	win_se_perwindow_clear( 'o', Outwin->number );
	win_se_hp_charset_select_normal();
}
/**************************************************************************
* term_se_soft_reset_hp
*	An event has occurred that has the side effect of doing a 
*	soft reset on an hp mode terminal.
**************************************************************************/
term_se_soft_reset_hp()
{
	/* NOT_on_HP   term_se_auto_wrap_off(); */
	term_se_cursor( 1 );
	/* NOT_to_HP   term_se_insert_mode( 0 ); */	/* not remembered */
	term_se_cursor_key_mode( 0 );
	term_se_appl_keypad_mode( 0 );
	/* NO_AFFECT term_se_keypad_xmit( 0 ); */
	term_se_scroll_region_normal();
	term_se_memory_unlock();
	term_se_set_margins_normal();
	term_se_character_set_normal();
	term_se_attribute( (FTCHAR) 0 );
	/* save_cursor is software */
	term_se_origin_mode( 0 );	/* not implemented */
	term_se_perwindow_clear( 'o' );
	term_se_hp_charset_select_normal();
}
/**************************************************************************
* win_se_soft_reset_hpansi
*	An event has occurred that has the side effect of doing a 
*	soft reset on an ansi mode hp window.
**************************************************************************/
win_se_soft_reset_hpansi()
{
	/* NOT ON HP win_se_auto_wrap_off(); */
	win_se_cursor( 1 );
	win_se_insert_mode( 0 );
	win_se_cursor_key_mode( 0 );
	win_se_appl_keypad_mode( 0 );
	/* NO_AFFECT win_se_keypad_xmit( 0 ); */
	win_se_scroll_region_normal();
	win_se_memory_unlock();
	win_se_margins_full();
	win_se_character_set_normal();
	win_se_attribute( (FTCHAR) 0 );
	init_save_cursor();
	win_se_origin_mode( 0 );	/* not implemented */
	win_se_perwindow_clear( 'o', Outwin->number );
	win_se_hp_charset_select_normal();
}
/**************************************************************************
* term_se_soft_reset_hpansi
*	An event has occurred that has the side effect of doing a 
*	soft reset on an ansi mode hp terminal.
**************************************************************************/
term_se_soft_reset_hpansi()
{
	/* NOT ON HP term_se_auto_wrap_off(); */
	term_se_cursor( 1 );
	term_se_insert_mode( 0 );	/* not remembered */
	term_se_cursor_key_mode( 0 );
	term_se_appl_keypad_mode( 0 );
	/* NO_AFFECT term_se_keypad_xmit( 0 ); */
	term_se_scroll_region_normal();
	term_se_memory_unlock();
	term_se_set_margins_normal();
	term_se_character_set_normal();
	term_se_attribute( (FTCHAR) 0 );
	/* save_cursor is software */
	term_se_origin_mode( 0 );	/* not implemented */
	term_se_perwindow_clear( 'o' );
	term_se_hp_charset_select_normal();
}
/**************************************************************************
* t_sync_terminal_mode
*	Syncronize the terminal (if necessary) to the terminal mode 
*	of window number "win".
*	Return 1 if the terminal mode was changed.  Also set the integer
*	pointed to by "p_clears" to 0 if the change did not clear the
*	screen or >0 if it did clear the screen.
*	Return 0 if the terminal already was in the correct mode.
*	Set the integer pointed to by "p_clears" to 
**************************************************************************/
t_sync_terminal_mode( win, p_clears )
	FT_WIN	*win;
	int	*p_clears;
{
	int	terminal_mode;
	int	terminal_mode_select;

	terminal_mode = win->terminal_mode;
	if ( terminal_mode != M_terminal_mode )
	{
		terminal_mode_select = 
		    Terminal_mode_from_to[ M_terminal_mode][ terminal_mode ];
		term_terminal_mode( terminal_mode_select, terminal_mode );
		*p_clears = F_terminal_mode_select_clear_screen[ 
						terminal_mode_select ];
		return( 1 );				/* changed */
	}
	return( 0 );					/* no change */
}
/**************************************************************************
* t_sync_terminal_mode_normal
*	Synchronize the terminal mode ( if necessary ) to the default
*	terminal mode.
**************************************************************************/
t_sync_terminal_mode_normal()
{
	int	terminal_mode_select;

	if ( F_terminal_mode_default != M_terminal_mode )
	{
		terminal_mode_select = 
		    Terminal_mode_from_to[ M_terminal_mode]
					 [ F_terminal_mode_default ];
		term_terminal_mode( terminal_mode_select, 
				    F_terminal_mode_default );
	}
}
/**************************************************************************
* term_terminal_mode
*	TERMINAL OUTPUT module for "terminal_mode_select".
**************************************************************************/
/* #include <termio.h> */
term_terminal_mode( terminal_mode_select, terminal_mode )
	int	terminal_mode_select;
	int	terminal_mode;
{
	char	*terminal_mode_str;
	char	buff[ 80 ];

	if ( terminal_mode_select < 0 )
	{
		term_beep();
		sprintf( buff, "terminal_mode_select=%d-%d", 
				terminal_mode_select, terminal_mode );
		error_record_msg( buff );
		return;
	}
	terminal_mode_str = T_terminal_mode_select[ terminal_mode_select ];
	if ( terminal_mode_str != NULL )
	{
		term_outgo();
		term_tputs( terminal_mode_str );
		term_outgo();
		M_terminal_mode = terminal_mode;
		M_pe = F_terminal_mode_personality[ terminal_mode ];
		switch ( F_terminal_mode_select_soft_reset[ 
							terminal_mode_select ] )
		{
		case SOFT_RESET_VT220:
			term_se_soft_reset();
			break;
		case SOFT_RESET_HP:
			term_se_soft_reset_hp();
			break;
		case SOFT_RESET_HPANSI:
			term_se_soft_reset_hpansi();
			break;
		}
		if ( F_terminal_mode_select_switch_page_0[
							terminal_mode_select ] )
			term_se_switch_page_number_0();
	}
}
/**************************************************************************
* term_check_terminal_mode
*	Check the terminal mode information against the the settings of
*	the real tty.  The input "tty_is_CS8" is 0 if the tty is in 
*	7 bit ( CS7 ) and is > 0 if the tty has an 8 bit path ( CS8 ).
*	If it is not in 8 bit, then 8 bit graphics and 8 bit control
*	characters will not make it to the terminal and 8 bit input is
*	not possible.  Correct the terminal description file to this
*	reality if the terminal description file claims differently.
**************************************************************************/
term_check_terminal_mode( tty_is_CS8 )
	int	tty_is_CS8;
{
	int	i;

	if ( tty_is_CS8 == 0 )
	{
		for ( i = 0; i < TERMINAL_MODE_MAX; i++ )
		{
			F_terminal_mode_is_control_8_bit[  i ] = 0;
			F_terminal_mode_is_graphics_8_bit[ i ] = 0;
		}
		F_graphics_8_bit = 0;
		F_input_8_bit = 0;
	}
}
/******************************************************************
* Syntax:
* 	terminal_mode_name-mode_number-mode_string=name_string
* Example:
* 	terminal_mode_name-0-CG=vt220-7
* Fields:
*	mode_number	a distinct terminal mode. 0 < mode_number < MAX_MODE(3)
*			This mode stays in effect until the next 
*			terminal_mode_name line.
*	mode_string	C = control 8 bit
*			G = graphics 8 bit
*	name_string	The name of the mode for the ^W m command
******************************************************************/
char *dec_encode();
extra_terminal_mode( buff, string, attr_on_string, attr_off_string ) 
	char	*buff;
	char	*string;
	char	*attr_on_string;
	char	*attr_off_string;
{
	char	*encoded;
	int	option;
	int	terminal_mode;
	int	i;

	if ( strcmp( buff, "terminal_mode_name" ) == 0 )
	{
		terminal_mode = get_terminal_mode_number( attr_on_string );
		if (  ( terminal_mode >= 0 ) 
		   && ( terminal_mode < TERMINAL_MODE_MAX ) )
		{
			if ( terminal_mode + 1 > T_terminal_mode_no )
				T_terminal_mode_no = terminal_mode + 1;
			strncpy( T_terminal_mode_name[ terminal_mode ], string, 
				TERMINAL_MODE_NAME_MAX );
			T_terminal_mode_name[ terminal_mode ]
					    [ TERMINAL_MODE_NAME_MAX ] = '\0';
			terminal_mode_name_modes( attr_off_string, 
						  terminal_mode );
			for( i = 0; i < TERMINAL_MODE_MAX; i++ )
				Terminal_mode_from_to[ i][ terminal_mode ] = -1;
		}
		else
		{
			printf( 
			"Invalid terminal_mode_name terminal_mode_number\n" );
		}
	}
	else if ( strcmp( buff, "terminal_mode_default" ) == 0 )
	{
		option = get_optional_value( string, 1 );
		F_terminal_mode_default = option;
		M_terminal_mode = option;
	}
	else if (  ( strcmp( buff, "terminal_mode_select" ) == 0 )
	        || ( strcmp( buff, "out_terminal_mode_switch" ) == 0 ) )
	{
	    int	doinstall;

	    if ( strcmp( buff, "out_terminal_mode_switch" ) == 0 )
		doinstall = 0;
	    else
		doinstall = 1;
	    if ( T_terminal_mode_select_no < TERMINAL_MODE_SELECT_MAX )
	    {
		F_terminal_mode_select_soft_reset[
					T_terminal_mode_select_no ] = 0;
		F_terminal_mode_select_switch_page_0[
					T_terminal_mode_select_no ] = 0;
		F_terminal_mode_select_clear_screen[
					T_terminal_mode_select_no ] = 0;
		for( i = 0; i < TERMINAL_MODE_MAX; i++ )
			Terminal_mode_select_from_to[
					T_terminal_mode_select_no ][ i ] = -1;
		if ( terminal_mode_select_mode_numbers( attr_on_string,
						   T_terminal_mode_select_no ) 
		     == 0 )
		{
			terminal_mode_select_se_encode( attr_off_string, 
						  T_terminal_mode_select_no );
			encoded = dec_encode( string );
			if ( doinstall )
				inst_terminal_mode_select( encoded, 
						   T_terminal_mode_select_no );
			T_terminal_mode_select[ T_terminal_mode_select_no ] =
								encoded; 
			T_terminal_mode_select_no++;
		}
		else
		{
			printf( 
			"terminal_mode_select has invalid terminal_mode\n" );
		}
	    }
	    else
	    {
			printf( 
			"Too many terminal_mode_select\n" );
	    }
	}
	else if ( strcmp( buff, "out_terminal_mode_select" ) == 0 )
	{
		if ( T_terminal_mode_select_no > 0 )
		{
			encoded = dec_encode( string );
			T_terminal_mode_select[ T_terminal_mode_select_no - 1 ]
								= encoded; 
		}
		else
		{
			printf( 
		"out_terminal_mode_select without terminal_mode_select\n" );
		}
	}
	else if ( strcmp( buff, "control_8_bit" ) == 0 )
	{
		for ( i = 0; i < TERMINAL_MODE_MAX; i++ )
			F_terminal_mode_is_control_8_bit[ i ] = 1;
	}
	else if ( strcmp( buff, "graphics_8_bit" ) == 0 )
	{
		F_graphics_8_bit = 1;
		for ( i = 0; i < TERMINAL_MODE_MAX; i++ )
			F_terminal_mode_is_graphics_8_bit[ i ] = 1;
	}
	else if ( strcmp( buff, "input_8_bit" ) == 0 )
	{
		F_input_8_bit = 1;
	}
	else
	{
		return( 0 );		/* no match */
	}
	return( 1 );
}
/**************************************************************************
* get_terminal_mode_number
*	Turn "string" into a terminal mode  mode number.
*	string		result
*	NULL		-1
*	"0"		0
*	..		..
*	"9"		9
*	otherwise	-1
**************************************************************************/
get_terminal_mode_number( string )
	char	*string;
{
	int	value;

	if ( string == ( (char *) 0 ) )
		return( -1 );
	if ( ( *string < '0' ) || ( *string  > '9' ) )
		return( -1 );
	value = *string - '0';
	if ( string[ 1 ] != '\0' )
		return( -1 );
	return( value );
}
/**************************************************************************
* terminal_mode_name_modes
*	Encode mode field in "string" from a 'terminal_mode_name=' line
*	and store the result under terminal mode number "terminal_mode".
*	C	= this terminal mode uses 8 bit controls.
*	G	= this teminal mode uses 8 bit graphics.
*	P0	= this terminal mode is personality # 0.
*	P1	= this terminal mode is personality # 1.
*	S	= this terminal mode uses scan codes.
*	.	= filler character for aligning columns.
*	other	= provokes a complaint.
**************************************************************************/
terminal_mode_name_modes( string, terminal_mode )
	char	*string;
	int	terminal_mode;
{
	char	*s;

	if ( string != ( (char *) 0 ) )
	{
		for ( s = string; *s != '\0'; s++ )
		{
		    if ( *s == 'C' )
		    {
			F_terminal_mode_is_control_8_bit[ terminal_mode ] = 1;
		    }
		    else if ( *s == 'G' )
		    {
			F_terminal_mode_is_graphics_8_bit[ terminal_mode ] = 1;
		    }
		    else if ( *s == 'P' )
		    {
			s++;
			if ( ( *s >= '0' ) || ( *s < '0' + MAX_PERSONALITY ) )
			{
				F_terminal_mode_personality[ terminal_mode ] = 
					*s - '0';
			}
			else if ( *s == '\0' )
			{
				printf( 
				"Missing terminal_mode_name personality #\n" );
				break;
			}
			else
			{
				printf( 
				    "Invalid terminal_mode_name mode P'%c'\n", 
				    *s );
			}
		    }
#ifdef SOFTPC
		    else if ( *s == 'S' )
			F_terminal_mode_is_scan_code[ terminal_mode ] = 1;
#endif
		    else if ( *s == '.' )
		    {
		    }
		    else
		    {
			printf( "Invalid terminal_mode_name mode '%c'\n", *s );
		    }
		}
	}
}
/**************************************************************************
* terminal_mode_select_mode_numbers
*	Encode the mode numbers field in "string" from a 'terminal_mode_select'
*	terminal description.  Store the result in terminal mode select
*	fields for terminal mode select number "terminal_mode_select_no".
*	*2	= this terminal mode select changes the terminal
*		  to terminal_mode 2 from any terminal_mode.
*	0232	= this terminal mode selcet changes the terminal
*		  to terminal mode 2 from terminal_mode 0 and
*		  to terminal mode 2 from terminal_mode 3.
*		  I.E pairs of from-to.
**************************************************************************/
terminal_mode_select_mode_numbers( string, terminal_mode_select_no )
	char	*string;
{
	char	*s;
	int	i;
	int	terminal_mode;
	int	from_terminal_mode;
	int	to_terminal_mode;

	if ( string == ( (char *) 0 ) )
		return( -1 );
	s = string;
	if ( *s == '*' )
	{
		s++;
		if ( ( *s >= '0' ) && ( *s <= '9' ) )
		{
			terminal_mode = *s - '0';
			if ( terminal_mode >= TERMINAL_MODE_MAX )
				return( -1 );
			/**************************************************
			* This terminal_mode_select command changes the
			* terminal mode to "terminal_mode" no matter what
			* terminal_mode it is currently in.
			* Also mark entire column that if you wish to get
			* to "terminal_mode" then use this terminal_mode_select
			* command.
			**************************************************/
			for ( i = 0; i < TERMINAL_MODE_MAX; i++ )
			{
				Terminal_mode_select_from_to[ 
					terminal_mode_select_no ][ i ] = 
								terminal_mode;
				Terminal_mode_from_to[ i ][ terminal_mode ] =
					terminal_mode_select_no;
			}
			return( 0 );
		}
		else
			return( -1 );
	}
	if ( *s == '\0' )
		return( -1 );
	while ( *s != '\0' )
	{
		if ( ( *s < '0' ) || ( *s > '9' ) )
			return( -1 );
		from_terminal_mode = *s - '0';
		if ( from_terminal_mode >= TERMINAL_MODE_MAX )
			return( -1 );
		s++;
		if ( ( *s < '0' ) || ( *s > '9' ) )
			return( -1 );
		to_terminal_mode = *s - '0';
		if ( to_terminal_mode >= TERMINAL_MODE_MAX )
			return( -1 );
			/**************************************************
			* This terminal_mode_select command changes the
			* terminal mode to "terminal_mode" no matter what
			* terminal_mode it is currently in.
			* Also mark entire column that if you wish to get
			* to "terminal_mode" then use this terminal_mode_select
			* command.
			**************************************************/
		/**********************************************************
		* If you are in terminal_mode "from_terminal_mode" and you
		* get this "terminal_mode_select_no", then the terminal
		* will change modes to terminal_mode "to_terminal_mode".
		**********************************************************/
		Terminal_mode_select_from_to[ terminal_mode_select_no ]
					    [ from_terminal_mode ] = 
							to_terminal_mode;
		/**********************************************************
		* If you wish to get from terminal_mode "from_terminal_mode"
		* (the first number of the pair) to terminal_mode
		* "to_terminal_mode" ( the second number of the pair), then
		* use terminal_mode_select "terminal_mode_select_no".
		* I.E. this one.
		**********************************************************/
		Terminal_mode_from_to[ from_terminal_mode ]
				     [ to_terminal_mode ] =
							terminal_mode_select_no;
		s++;
	}
	return( 0 );
}
/**************************************************************************
* terminal_mode_select_se_encode
*	Encode the side effects field in "string" from the
*	'terminal_mode_select' line from the terminal description file.
*	Store the result in the fields for terminal mode select command
*	number "terminal_mode_select_no".
*	R	= mode selection causes vt220 type soft reset.
*	H	= mode selection causes HP type soft reset.
*	A	= mode selection causes HP ANSI type soft reset.
*	P	= mode selection causes multipage terminal to switch to 
*		  page number 0.
*	C	= mode selection causes screen to be cleared.
*	.	= filler character - ignored.
*	other	= provokes complaint.
**************************************************************************/
terminal_mode_select_se_encode( string, terminal_mode_select_no )
	char	*string;
	int	terminal_mode_select_no;
{
	char	*s;

	if ( string != ( (char *) 0 ) )
	{
		for ( s = string; *s != '\0'; s++ )
		{
		    if ( *s == 'R' )
			F_terminal_mode_select_soft_reset[
				terminal_mode_select_no ] = SOFT_RESET_VT220;
		    else if ( *s == 'H' )
			F_terminal_mode_select_soft_reset[
				terminal_mode_select_no ] = SOFT_RESET_HP;
		    else if ( *s == 'A' )
			F_terminal_mode_select_soft_reset[
				terminal_mode_select_no ] = SOFT_RESET_HPANSI;
		    else if ( *s == 'P' )
			F_terminal_mode_select_switch_page_0[
						terminal_mode_select_no ] = 1;
		    else if ( *s == 'C' )
			F_terminal_mode_select_clear_screen[
						terminal_mode_select_no ] = 1;
		    else if ( *s == '.' )
		    {
		    }
		    else
		    {
			printf( "Invalid terminal_mode_select mode '%c'\n", 
								*s );
		    }
		}
	}
}
/**************************************************************************
* term_init_terminal_mode
*	Initialize remembered terminal mode personality to the personality
*	associated with the default terminal mode.
**************************************************************************/
term_init_terminal_mode()
{
	M_pe = F_terminal_mode_personality[ F_terminal_mode_default ];
}
/**************************************************************************
* init_windows_terminal_mode
*	Init the modes for the window "Outwin" to those modes associated
*	with the default terminal mode.
**************************************************************************/
init_windows_terminal_mode()
{
	Outwin->terminal_mode = F_terminal_mode_default;
	Outwin->control_8_bit_on = 
		F_terminal_mode_is_control_8_bit[ F_terminal_mode_default ];
	Outwin->graphics_8_bit = 
		F_terminal_mode_is_graphics_8_bit[ F_terminal_mode_default ];
	Outwin->personality =
		F_terminal_mode_personality[ F_terminal_mode_default ];
	O_pe = Outwin->personality;
#ifdef SOFTPC
	Outwin->terminal_mode_is_scan_code =
		F_terminal_mode_is_scan_code[ F_terminal_mode_default ];
#else
	Outwin->terminal_mode_is_scan_code = 0;
#endif
}
/**************************************************************************
* modes_init_terminal_mode
*	Init the modes for the window "win" to those modes associated
*	with the default terminal mode.
**************************************************************************/
modes_init_terminal_mode( win )
	FT_WIN	*win;
{
	win->terminal_mode = F_terminal_mode_default;
	win->control_8_bit_on = 
		F_terminal_mode_is_control_8_bit[ F_terminal_mode_default ];
	win->graphics_8_bit = 
		F_terminal_mode_is_graphics_8_bit[ F_terminal_mode_default ];
	win->personality =
		F_terminal_mode_personality[ F_terminal_mode_default ];
	if ( win == Outwin )
		O_pe = Outwin->personality;
#ifdef SOFTPC
	win->terminal_mode_is_scan_code =
		F_terminal_mode_is_scan_code[ F_terminal_mode_default ];
#endif
}

char	*Text_terminal_modes =
	"Terminal modes:";
/**************************************************************************
* get_terminal_mode_mode
*	Return information necessary for the ^Wm module to determine
*	if the terminal mode is setable by the user and to prompt
*	for the new choice of mode.
*	"p_prompt" is used as a pointer to set a pointer to the prompt
*		field.
*	"p_index" is used as a pointer to set the current mode.
*	"p_max_index" is used as a pointer to set the maximum mode allowed.
*	"names" is an array of character pointers set to the names of the
*		modes.
* Returns
*	0	mode prompts all set up.
*	-1	mode not used on this terminal description.
**************************************************************************/
get_terminal_mode_mode( p_prompt, p_index, p_max_index, names )
	char	**p_prompt;
	int	*p_index;
	int	*p_max_index;
	char	**names;
{
	int	i;

	if ( T_terminal_mode_no >= 2 )
	{
					/* "Terminal modes:" */
		*p_prompt = Text_terminal_modes;
		*p_index = Outwin->terminal_mode;
		*p_max_index = T_terminal_mode_no - 1;
		for( i = 0; i < T_terminal_mode_no; i++ )
			names[ i ] = T_terminal_mode_name[ i ];
		return( 0 );
	}
	return( -1 );
}
/**************************************************************************
* put_terminal_mode_mode
*	User has selected the mode with the index "index" using the ^Wm
*	command. Set both the window and the terminal to the new mode.
**************************************************************************/
put_terminal_mode_mode( index )
	int	index;
{
	int	clears;
	int	terminal_mode_select;

	terminal_mode_select = 
	    Terminal_mode_from_to[ Outwin->terminal_mode][ index ];
	Outwin->terminal_mode = index;
	Outwin->control_8_bit_on = F_terminal_mode_is_control_8_bit[ index ];
	Outwin->graphics_8_bit = F_terminal_mode_is_graphics_8_bit[ index ];
	Outwin->personality = F_terminal_mode_personality[ index ];
	O_pe = Outwin->personality;
#ifdef SOFTPC
	Outwin->terminal_mode_is_scan_code =
		F_terminal_mode_is_scan_code[ index ];
#endif
	if ( F_terminal_mode_select_clear_screen[ terminal_mode_select ] )
		win_se_clear_screen();
	switch ( F_terminal_mode_select_soft_reset[ terminal_mode_select ] )
	{
	case SOFT_RESET_VT220:
		win_se_soft_reset();
		break;
	case SOFT_RESET_HP:
		win_se_soft_reset_hp();
		break;
	case SOFT_RESET_HPANSI:
		win_se_soft_reset_hpansi();
		break;
	}
	if ( t_sync_terminal_mode( Outwin, &clears ) )
			t_sync_auto_wrap( 1 );
	if (  ( Outwin->onscreen )
	   && ( Outwin->fullscreen == 0 )
	   && clears )
	{
		split_screen_incompatible( clears );
	}
}
