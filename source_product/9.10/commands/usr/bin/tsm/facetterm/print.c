/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: print.c,v 70.1 92/03/09 15:46:20 ssa Exp $ */
/**************************************************************************
* print.c
*	Transparent print and hp print modules
**************************************************************************/
#include <stdio.h>
#include "ftproc.h"
#include "ftwindow.h"
#include "features.h"
#include "decode.h"

char	*T_transparent_print_on = NULL;
char	*T_transparent_print_off = NULL;
char	*T_out_transparent_print = NULL;
char	*T_out_hp_transparent_print = NULL;
int	M_transparent_print_on = 0;
int	F_transparent_print = 0;

#include "print.h"
UNCHAR	Raw_transparent_print_off[ 20 ] = "";
int	Len_transparent_print_off = 0;

/**************************************************************************
* dec_transparent_print_on
*	DECODE module for 'transparent_print_on'.
*	Turn transparent print mode on.
**************************************************************************/
dec_transparent_print_on()
{
	fct_transparent_print_on();
}
/**************************************************************************
* inst_transparent_print_on
*	INSTALL module for 'transparent_print_on'.
**************************************************************************/
inst_transparent_print_on( str )
	char	*str;
{
	dec_install( "transparent_print_on", (UNCHAR *) str,
					dec_transparent_print_on, 0, 0,
					(char *) 0 );
}
#include "readwindow.h"
/**************************************************************************
* fct_transparent_print_on
*	ACTION module for 'transparent_print_on'.
**************************************************************************/
fct_transparent_print_on()
{
	Outwin->transparent_print_on = 1;
	term_transparent_print_on();
	Read_window_max[ Outwin->number ] = Transparent_print_read_window_max;
}
/**************************************************************************
* term_transparent_print_on
*	TERMINAL OUTPUT module for 'transparent_print_on'.
**************************************************************************/
term_transparent_print_on()
{
	if ( T_transparent_print_on != NULL )
		term_tputs( T_transparent_print_on );
	M_transparent_print_on = 1;
}

/**************************************************************************
* dec_transparent_print_off
*	DECODE module for 'transparent_print_off'.
*	Turn transparent print mode off.
**************************************************************************/
dec_transparent_print_off()
{
	fct_transparent_print_off();
}
/**************************************************************************
* inst_transparent_print_off
*	INSTALL module for 'transparent_print_off'.
**************************************************************************/
inst_transparent_print_off( str )
	char	*str;
{
	dec_install( "transparent_print_off", (UNCHAR *) str,
					dec_transparent_print_off, 0, 0,
					(char *) 0 );
}
/**************************************************************************
* fct_transparent_print_off
*	ACTION module for 'transparent_print_off'.
**************************************************************************/
fct_transparent_print_off()
{
	Outwin->transparent_print_on = 0;
	term_transparent_print_off();
	Read_window_max[ Outwin->number ] = READ_WINDOW_MAX;
	check_print_inactive( Outwin->number );
}
/**************************************************************************
* term_transparent_print_off
*	TERMINAL OUTPUT module for 'transparent_print_off'.
**************************************************************************/
term_transparent_print_off()
{
	if ( T_transparent_print_off != NULL )
		term_tputs( T_transparent_print_off );
	M_transparent_print_on = 0;
}
/**************************************************************************
* transparent_print_is_on_winno
*	Return > 0 if window number "winno" is in transparent print mode.
*	Otherwise 0.
**************************************************************************/
transparent_print_is_on_winno( winno )
	int	winno;
{
	return( Wininfo[ winno ]->transparent_print_on );
}
/**************************************************************************
* init_windows_transparent_print
*	Startup.
**************************************************************************/
init_windows_transparent_print()
{
	Outwin->transparent_print_on = 0;
}
#include "printer.h"
/**************************************************************************
* modes_init_transparent_print
*	Window "win" went idle.
**************************************************************************/
modes_init_transparent_print( win )
	FT_WIN	*win;
{
	win->transparent_print_on = 0;
	if ( Window_printer[ win->number ] )
	    Read_window_max[ win->number ] = Transparent_print_read_window_max;
	else
	{
	    Read_window_max[ win->number ] = READ_WINDOW_MAX;
	    check_print_inactive( win->number );
	}
}
char	*Text_transparent_print =
	"Trans print:";
char	*Text_transparent_print_off =
	"OFF";
char	*Text_transparent_print_on =
	"ON";
/**************************************************************************
* get_transparent_print_mode
*	Return information for  ^W m  command to inquire and set
*	transparent print mode.
**************************************************************************/
get_transparent_print_mode( p_prompt, p_index, p_max_index, names )
	char	**p_prompt;
	int	*p_index;
	int	*p_max_index;
	char	**names;
{
	if ( F_transparent_print )
	{
		/* "Trans print:" */
		*p_prompt = Text_transparent_print;
		*p_index = Outwin->transparent_print_on;
		*p_max_index = 1;
		names[ 0 ] = Text_transparent_print_off; /* "OFF" */
		names[ 1 ] = Text_transparent_print_on;  /* "ON"  */
		return( 0 );
	}
	return( -1 );
}
/**************************************************************************
* put_transparent_print_mode
*	The user has used the  ^W m  command to put transparent print
*	in the "index" state. 0=off 1=on.
**************************************************************************/
put_transparent_print_mode( index )
	int	index;
{
	Outwin->transparent_print_on = index;
	if ( index )
		Read_window_max[ Outwin->number ] =
				Transparent_print_read_window_max;
	else
	{
		Read_window_max[ Outwin->number ] = READ_WINDOW_MAX;
		check_print_inactive( Outwin->number );
	}
}
#include "max_buff.h"
/**************************************************************************
* term_out_transparent_print
*	Internally generated command.
*	Output the string "ptr" of "count" characters to the terminal
*	in transparent print mode.
**************************************************************************/
term_out_transparent_print( ptr, count )
	char	*ptr;
	int	count;
{
	char	buff[ MAX_BUFF + 1 ];
	int	max;
	int	parm[ 2 ];			/* 1 used - must be >= 2 - %i */
	char    *string_parms[ 1 ];
	char	*my_tparm();
	char	*p;

	if ( T_out_transparent_print != NULL )
	{
		max = count;
		if ( max > MAX_BUFF )
			max = MAX_BUFF;
		strncpy( buff, ptr, max );
		buff[ max ] = '\0';
		string_parms[ 0 ] = buff;
		p = my_tparm( T_out_transparent_print, parm, string_parms, -1 );
		my_tputs( p, 1 );
		term_outgo();
		term_drain();
		M_transparent_print_on = 0;
	}
	else if (  ( T_transparent_print_on != NULL )
		&& ( T_transparent_print_off != NULL ) )
	{
		term_tputs( T_transparent_print_on );
		term_write( ptr, count );
		term_tputs( T_transparent_print_off );
		term_outgo();
		term_drain();
		M_transparent_print_on = 0;
	}
}
/**************************************************************************
* term_out_hp_transparent_print
*	Internally generated command.
*	Output the string "ptr" of "count" characters to the terminal
*	in transparent print mode on an HP terminal.
**************************************************************************/
term_out_hp_transparent_print( ptr, count )
	char	*ptr;
	int	count;
{
	int	parm[ 2 ];			/* 1 used - must be >= 2 - %i */
	char    *string_parms[ 1 ];
	char	*my_tparm();
	char	*p;

	if ( T_out_hp_transparent_print != NULL )
	{
		parm[ 0 ] = count;
		string_parms[ 0 ] = ptr;
		p = my_tparm( T_out_hp_transparent_print, parm, string_parms,
									-1 );
		my_tputs( p, 1 );
		term_outgo();
	}
}

#include "tpnotify.h"
int	Hp_transparent_print_reply = 0;
int	sF_hp_transparent_print_enq_ack = { 0 };
#include "printtimer.h"
/****************************************************************************/
char *dec_encode();
/**************************************************************************
* extra_transparent_print
*	TERMINAL DESCRIPTION PARSER module for 'transparent_print'.
**************************************************************************/
/*ARGSUSED*/
extra_transparent_print( buff, string, attr_on_string, attr_off_string ) 
	char	*buff;
	char	*string;
	char	*attr_on_string;		/* not used */
	char	*attr_off_string;		/* not used */
{
	if ( strcmp( buff, "transparent_print_off" ) == 0 )
	{
		T_transparent_print_off = dec_encode( string );
		inst_transparent_print_off( T_transparent_print_off );
		F_transparent_print = 1;
		strcpy( (char *) Raw_transparent_print_off, 
				 T_transparent_print_off );
		Len_transparent_print_off = 
				strlen( (char *) Raw_transparent_print_off );
	}
	else if ( strcmp( buff, "transparent_print_on" ) == 0 )
	{
		T_transparent_print_on = dec_encode( string );
		inst_transparent_print_on( T_transparent_print_on );
	}
	else if ( strcmp( buff, "out_transparent_print" ) == 0 )
	{
		T_out_transparent_print = dec_encode( string );
	}
	else if ( strcmp( buff, "out_hp_transparent_print" ) == 0 )
	{
		T_out_hp_transparent_print = dec_encode( string );
		Hp_transparent_print_reply = 1;
	}
	else if ( strcmp( buff, "hp_transparent_print_enq_ack" ) == 0 )
	{
		xF_hp_transparent_print_enq_ack = 1;
	}
	else if ( strcmp( buff, "transparent_print_quiet_timer" ) == 0 )
	{
		int	x;

		if (  ( string != (char *) 0 )
		   && ( ( x = atoi( string ) ) > 0 ) )
		{
			Transparent_print_quiet_timer = x;
		}
		else
			printf( "Invalid transparent_print_quiet_timer\n" );
	}
	else if ( strcmp( buff, "transparent_print_idle_timer" ) == 0 )
	{
		int	x;

		if (  ( string != (char *) 0 )
		   && ( ( x = atoi( string ) ) > 0 ) )
		{
			Transparent_print_idle_timer = x;
			Transparent_print_short_timer = x;
		}
		else
			printf( "Invalid transparent_print_idle_timer\n" );
	}
	else if ( strcmp( buff, "transparent_print_idle_timer_max" ) == 0 )
	{
		int	x;

		if (  ( string != (char *) 0 )
		   && ( ( x = atoi( string ) ) > 0 ) )
		{
			Transparent_print_idle_timer_max = x;
		}
		else
			printf( "Invalid transparent_print_idle_timer_max\n" );
	}
	else if ( strcmp( buff, "transparent_print_full_timer" ) == 0 )
	{
		int	x;

		if (  ( string != (char *) 0 )
		   && ( ( x = atoi( string ) ) > 0 ) )
		{
			Transparent_print_full_timer = x;
		}
		else
			printf( "Invalid transparent_print_full_timer\n" );
	}
	else if ( strcmp( buff, "transparent_print_delay_count" ) == 0 )
	{
		int	x;

		if (  ( string != (char *) 0 )
		   && ( ( x = atoi( string ) ) > 0 ) )
		{
			Transparent_print_delay_count = x;
		}
		else
			printf( "Invalid transparent_print_delay_count\n" );
	}
	else if ( strcmp( buff, "transparent_print_timers_disable" ) == 0 )
	{
		Transparent_print_timers_disable = 1;
	}
	else
	{
		return( 0 );		/* no match */
	}
	return( 1 );
}
