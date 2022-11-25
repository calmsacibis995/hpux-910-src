/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: keystroke.c,v 66.6 90/10/18 09:24:05 kb Exp $ */
/**************************************************************************
* keystroke.c
*	Record keystrokes in a file for later replay.
**************************************************************************/
#include <errno.h>
extern int errno;
#include "keystroke.h"
/**************************************************************************
* keystroke_capture_timeout
*	Record a timeout in the keystroke capture file.
**************************************************************************/
keystroke_capture_timeout()
{
	char	*p;
	char	*visible_octal();

	p = visible_octal( "\377\000", 2, 0 );
	write( Keystroke_capture_fd, p, strlen( p ) );
}
/**************************************************************************
* keystroke_capture_key
*	Record the string "s" of length "len" in the keystroke capture file.
**************************************************************************/
keystroke_capture_key( s, len )
	char	*s;
	int	len;
{
	char	*p;
	char	*visible();

	keystroke_capture_time();
	p = visible( s, len, 1 );
	write( Keystroke_capture_fd, p, strlen( p ) );
}
/**************************************************************************
* keystroke_capture_time
*	Record a timing mark in the keystroke capture file if more than
*	a second has elapsed.
**************************************************************************/
keystroke_capture_time()
{
	long	time();
	long	now;
	char	*p;
	char	*visible();
	char	*visible_octal();
	int	diff;
	char	c;

	now = time( (long *) 0 );
	diff = now - Keystroke_capture_time;
	if ( diff > 1 )
	{
		if ( diff > 20 )
			diff = 20;
		p = visible( "\377", 1, 0 );
		write( Keystroke_capture_fd, p, strlen( p ) );
		c = diff;
		p = visible_octal( &c, 1, 0 );
		write( Keystroke_capture_fd, p, strlen( p ) );
	}
	Keystroke_capture_time = now;
}
/**************************************************************************
* keystroke_capture_end
*	Prepare for closing keystroke capture file.
**************************************************************************/
keystroke_capture_end()
{
	visible_end();
	write( Keystroke_capture_fd, "\n", 1 );
}
/**************************************************************************
* keystroke_play_read
*	Return the next keystroke play character at the pointer "p";
*	and return 1.
*	If a timeout is recorded at this spot in the keystroke capture file,
*	return 0.
*	If the keystroke replay was canceled or an error occurred,
*	close the keystroke replay file and return 0.
**************************************************************************/
keystroke_play_read( p )
	char	*p;
{
	char	c;
	int	status;

	while ( 1 )
	{
		if ( read( 0, &c, 1 ) > 0 )
		{
			close_keystroke_play();
			return( 0 );
		}
		status = read( Keystroke_play_fd, &c, 1 );
		if ( status <= 0 )
		{
			close_keystroke_play();
			return( 0 );
		}
		else if ( ( c & 0x00FF ) == 0x00FF )
		{
			status = read( Keystroke_play_fd, &c, 1 );
			if ( status <= 0 )
			{
				close_keystroke_play();
				return( 0 );
			}
			else if ( c == 0 )
			{
				return( 0 );
			}
			else if ( ( c & 0x00FF ) == 0x00FF )
			{
				*p = c;
				return( 1 );
			}
			else
				sleep( c );
		}
		else
		{
			*p = c;
			return( 1 );
		}
	}
}
/**************************************************************************
* keystroke_play_read_receive
* Returns:
*	1  = character in "p"
*	0  = stopped by key on keyboard or error - replay must be off.
*	-1 = interrupted - errno must be EINTR
**************************************************************************/
/*ARGSUSED*/
keystroke_play_read_receive( p, max )
	char	*p;
	int	max;			/* not used - only one returned */
{
	int	cc;
	char	c;
	int	status;

	while ( 1 )
	{
		/**********************************************************
		* If keyboard was pressed, stop replay.
		**********************************************************/
		if ( read( 0, &c, 1 ) > 0 )
		{
			keystroke_play_cancel();
			return( 0 );
		}
		/**********************************************************
		* Read the keystroke replay file
		**********************************************************/
		status = read( Keystroke_play_fd, &c, 1 );
		if ( status <= 0 )
		{
			/**************************************************
			* Stop if error
			**************************************************/
			if ( ( status == -1 ) && ( errno == EINTR ) )
				return( -1 );
			keystroke_play_cancel();
			return( 0 );
		}
		else if ( ( c & 0x00FF ) == 0x00FF )
		{
			/**************************************************
			* 0xFF or \377 in file is special.
			**************************************************/
			status = read( Keystroke_play_fd, &c, 1 );
			if ( status <= 0 )
			{
				/******************************************
				* stop on error
				******************************************/
				keystroke_play_cancel();
				return( 0 );
			}
			else if ( c == 0 )
			{
				/******************************************
				* 0xFF 0x00  is a timeout.  This should not
				* have been recorded and is a bug or an
				* erroneously hand edited key file. Stop
				* replay.
				******************************************/
				keystroke_play_cancel();
				return( 0 );
			}
			else if ( ( c & 0x00FF ) == 0x00FF )
			{
				/******************************************
				* 0xFF 0xFF  is a single 0xFF.
				******************************************/
				*p = c;
				return( 1 );
			}
			else
			{
				/******************************************
				* 0xFF and 0x01 to 0xFE is a sleep.
				* Interrupt is from sender.
				******************************************/
				cc = c;
				if ( ( sleep( cc ) > 0 ) && ( errno == EINTR ) )
					return( -1 );
			}
		}
		else
		{
			*p = c;
			return( 1 );
		}
	}
}
