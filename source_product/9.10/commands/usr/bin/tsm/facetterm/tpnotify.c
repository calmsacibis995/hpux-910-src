/*****************************************************************************
** Copyright (c) 1990        Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: tpnotify.c,v 66.8 90/10/18 09:27:53 kb Exp $ */
/**************************************************************************
* tpnotify.c
*	Support hp transparent print notification.
**************************************************************************/

#include "person.h"
#include "tpnotify.h"

int	Transparent_print_notify = 0;	/* 1 = notify in progress */
char	*Transparent_print_ptr = (char *) 0;
int	Transparent_print_count = 0;
int	Transparent_print_winno = 0;

#include "options.h"
#include "printer.h"

/**************************************************************************
* transparent_print_notify
*	Since the terminal talks back to program on a print command,
*	we must make the receiver quit reading the keyboard.
*	Store the string "ptr", its length "count", and the window
*	which is doing the print in globals.
*	Signal the receiver.
*	The actual print is done in chk_transparent_print_notify
*	which is called below fsend_get_acks_only when the receiver
*	acks the signal.
**************************************************************************/
transparent_print_notify( ptr, count, winno )
	char	*ptr;
	int	count;
	int	winno;
{
	if ( Window_printer_disable )
		return;
	Transparent_print_notify = 1;
	Transparent_print_ptr = ptr;
	Transparent_print_count = count;
	Transparent_print_winno = winno;
	send_packet_signal();
	while( Transparent_print_notify > 0 )
		fsend_get_acks_only();
}
/**************************************************************************
* chk_transparent_print_notify
*	If a transparent print is pending, execute it and return 1.
*	Otherwise return 0.
*	If the print fails, turn off print on the window and interrupt
*	the print program.
**************************************************************************/
char *Text_printer_mode_terminated =
				"\nPrinter mode terminated by break.\n";
chk_transparent_print_notify()
{

	if ( Transparent_print_notify )
	{
		if ( hp_transparent_print() < 0 )
		{

			Window_printer_disable = 1;
			puts_winno( Text_printer_mode_terminated,
				    Transparent_print_winno );
			fsend_int_to_winno( Transparent_print_winno );
		}
		fct_window_mode_ans_curwin();
		Transparent_print_notify = 0;
		return( 1 );
	}
	else
	{
		return( 0 );
	}
}
/**************************************************************************
* hp_transparent_print
*	Negotiate with an HP terminal to get through to its printer
*	and to field the various replys which cannot be eliminated.
*	The user may hit break to get out of a deadlock if the 
*	printer is not working,
*	so abandon the effort on a break.
**************************************************************************/
hp_transparent_print()
{
	int	c;
	int	tenths;
	int	key_so_far;

	tenths = 20;
	term_out_hp_transparent_print( Transparent_print_ptr,
				       Transparent_print_count );
	if ( mF_hp_transparent_print_enq_ack )
	{
		term_puts( "\005" );			/* enq */
		while( 1 )
		{
			c = ftrawread();
			if ( c == '\006' )
				break;			/* ack */
			else if (  ( c == 0 )
				&& ( Opt_use_PARMRK_for_break == 0 ) )
			{
				/******************************************
				* User hit break;
				******************************************/
				return( -1 );
			}
			else if (  ( c == 0xFF ) 
				&& ( Opt_use_PARMRK_for_break ) )
			{				/* may be break */
				c = ftrawread();
				if ( c != 0 )
				{
					stray_key_to_receiver( 0xFF );
					stray_key_to_receiver( c );
					continue;
				}
				c = ftrawread();
				if ( c != 0 )
				{
					stray_key_to_receiver( 0xFF );
					stray_key_to_receiver( 0 );
					stray_key_to_receiver( c );
					continue;
				}
				/******************************************
				* User hit break;
				******************************************/
				return( -1 );
			}
			else
				stray_key_to_receiver( c );
		}
	}
	term_write( Transparent_print_ptr, Transparent_print_count );
	key_so_far = 0;
	while( 1 )
	{
		c = ftrawread();
		if ( key_so_far )
		{
			if ( c == '\r' )
			{
				if ( key_so_far == 'S' )
					return( 0 );
				else if ( key_so_far == 'F' )
					return( -1 );
				else if ( key_so_far == 'U' )
					return( -1 );
			}
			stray_key_to_receiver( key_so_far );
			key_so_far = 0;
		}
		if (  ( c == 'S' ) 
		   || ( c == 'F' ) 
		   || ( c == 'U' ) )
		{
			key_so_far = c;
		}
		else if (  ( c == 0 )
			&& ( Opt_use_PARMRK_for_break == 0 ) )
		{				/* break */
			/* break */
			/**************************************************
			* User hit break;
			**************************************************/
			return( -1 );
		}
		else if (  ( c == 0xFF ) 
			&& ( Opt_use_PARMRK_for_break ) )
		{				/* may be break */
			c = ftrawread();
			if ( c != 0 )
			{
				stray_key_to_receiver( 0xFF );
				stray_key_to_receiver( c );
				continue;
			}
			c = ftrawread();
			if ( c != 0 )
			{
				stray_key_to_receiver( 0xFF );
				stray_key_to_receiver( 0 );
				stray_key_to_receiver( c );
				continue;
			}
			/**************************************************
			* User hit break;
			**************************************************/
			/* break */
			return( -1 );
		}
		else
			stray_key_to_receiver( c );
		
	}
}
/**************************************************************************
* stray_key_to_receiver
*	The key "c" was intercepted by the sender in trying to get
*	the print reply and should really go to the receiver.
**************************************************************************/
stray_key_to_receiver( c )
	int	c;
{
	fct_window_mode_ans( 't' );
	fct_window_mode_ans( (char) c );
}
