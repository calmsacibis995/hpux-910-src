/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: fttermio.c,v 70.1 92/03/09 15:43:56 ssa Exp $ */
#include <stdio.h>
/**************************************************************************
* fttermio.c 
*	Operations setting up the line discipline on the real tty.
**************************************************************************/

#include <termio.h>

#ifndef B19200
#define B19200 EXTA
#endif
#ifndef B38400
#define B38400 EXTB
#endif

						/* not FT_USE_TERMIOS */
struct termio T_normal;
struct termio T_window;
struct termio T_window_timed;
struct termio T_normal_master;
struct termio T_normal_master_off;
						/* not FT_USE_TERMIOS */


#include "bsdtty.h"

struct ltchars T_ltchars_master;
						/* HAS_LTCHARS */

#include "baud.h"
int	Ft_baud = 0;
int	Ft_xon_xoff = 0;

/**************************************************************************
* termio_init
*	Record the terminal settings that the user has been using and set
*	up termino structures for FacetTerm operation.
*	"use_xon_xoff" is non zero if the terminal should be run in
*		'ixon -ixany' mode
*	"force_CS8" ins non zero if the terminal should be run in 
*		'cs8 -parenb'
*	Terminals forced to 8 bit mode will have 'istrip' cleared unless
*		"no_clear_ISTRIP" is non zero.
*	"use_PARMRK_for_break" is non zero if break detection can be
*		reliably done with PARMRK.
**************************************************************************/
/* #define DEBUG_SUN */
termio_init( use_xon_xoff, force_CS8, no_clear_ISTRIP, use_PARMRK_for_break,
							allow_tabs )
	int	use_xon_xoff;
	int	force_CS8;
	int	no_clear_ISTRIP;
	int	use_PARMRK_for_break;
	int	allow_tabs;
{
	int	status;
	int	baud;

	status = ioctl( 1, TCGETA, &T_normal );
	if ( status < 0 )
	{
		perror( "ioctl TCGETA failed" );
		return( -1 );
	}
	T_window = T_normal;
						/* PSEUDOTTY */
	T_normal_master = T_normal;
	T_normal_master.c_line = 0;
	/******************************************************************
	* Some HP applications fail if TAB3 is set.
	******************************************************************/
	T_normal_master_off = T_normal_master;
	T_normal_master_off.c_lflag &= ( ~ ECHO );

	status = ioctl( 1, TIOCGLTC, &T_ltchars_master );
	if ( status < 0 )
	{
		perror( "ioctl TIOCGLTC failed" );
		return( -1 );
	}
						/* PSEUDOTTY */

	baud = T_normal.c_cflag & CBAUD;
	switch( baud )
	{
	case B38400:	Ft_baud = -2;	break;
	case B19200:	Ft_baud = -1;	break;
	case B9600:	Ft_baud =  0;	break;
	case B4800:	Ft_baud =  1;	break;
	case B2400:	Ft_baud =  2;	break;
	case B1200:	Ft_baud =  3;	break;
	case B600:	Ft_baud =  4;	break;
	case B300:	Ft_baud =  5;	break;
	default:	printf( "WARNING - Unknown baud rate %x\n", baud );
			Ft_baud =  0;	break;
	}
	if ( use_PARMRK_for_break )
		T_window.c_iflag |=  ( PARMRK );
	else
		T_window.c_iflag &= ~( PARMRK );
	T_window.c_iflag |=  ( 0 );
	T_window.c_iflag &= ~( IGNBRK | BRKINT |
				INLCR | IGNCR | ICRNL | IUCLC | IXANY | IXOFF );
	if ( force_CS8 && ( no_clear_ISTRIP == 0 ) )
		T_window.c_iflag &= ~( ISTRIP );
	if ( use_xon_xoff )
	{
		T_window.c_iflag |= ( IXON );
		Ft_xon_xoff = 1;
	}
	else
		T_window.c_iflag &= ~( IXON );

	T_window.c_oflag |=  ( 0 );
	T_window.c_oflag &= ~( OPOST | OLCUC | ONLCR | OCRNL | ONOCR | ONLRET
			     | OFILL | OFDEL | TAB3 );

	if ( force_CS8 )
	{
		T_window.c_cflag &= ~( PARENB | CSIZE );
		T_window.c_cflag |=  ( CS8 );
	}

	T_window.c_lflag |=  ( NOFLSH );
	T_window.c_lflag &= ~( ISIG | ICANON | XCASE | ECHO | ECHOE | ECHOK );
	T_window.c_cc[ VMIN ] = 1;
	T_window.c_cc[ VTIME ] = 1;
	if (  ( ( T_window.c_cflag & CSIZE ) == CS8 )
	   && ( no_clear_ISTRIP == 0 ) )
	{
		T_window.c_iflag &= ~( ISTRIP );
	}
	T_window_timed = T_window;
	setvbuf( stdout, NULL, _IOFBF, BUFSIZ );
	if ( ( T_window.c_cflag & CSIZE ) == CS8 )
		return( 1 );				/* 8 bit path */
	else
		return( 0 );				/* 7 bit path */
}
/**************************************************************************
* translate_baud
*	Translate the line discipline baud rate field "baud" to x where 
*	the transmission rate is 2 ** x milliseconds per character.
**************************************************************************/
translate_baud( baud )
	int	baud;
{
	int	trans_baud;

	switch( baud )
	{
	case 38400:	trans_baud = -2;		break;
	case 19200:	trans_baud = -1;		break;
	case 9600:	trans_baud =  0;		break;
	case 4800:	trans_baud =  1;		break;
	case 2400:	trans_baud =  2;		break;
	case 1200:	trans_baud =  3;		break;
	case 600:	trans_baud =  4;		break;
	case 300:	trans_baud =  5;		break;
	default:	trans_baud = INVALID_BAUD_RATE;	break;
	}
	return( trans_baud );
}
/**************************************************************************
* termio_window
*	Set the real tty to the line discipline modes appropriate for
*	window command mode when not timing.
**************************************************************************/
termio_window()
{
	int	status;

	term_outgo();
	status = ioctl( 1, TCSETAW, &T_window );
	if ( status < 0 )
	{
		perror( "ioctl TCSETAW window failed" );
		return( -1 );
	}
	return( 0 );
}
/**************************************************************************
* termio_window_timed
*	Set the real tty to the line discipline modes appropriate for
*	window command mode when timing for the next character.
*	"min" and "tenths" are as defined by the line discipline.
**************************************************************************/
termio_window_timed( min, tenths )
	int	min;
	int	tenths;
{
	int	status;

	term_outgo();
	T_window_timed.c_cc[ VMIN ] = min;
	T_window_timed.c_cc[ VTIME ] = tenths;
	status = ioctl( 1, TCSETAW, &T_window_timed );
	if ( status < 0 )
	{
		perror( "ioctl TCSETAW window_timed failed" );
		return( -1 );
	}
	return( 0 );
}
/**************************************************************************
* termio_normal
*	Set the real tty to the line discipline modes appropriate for
*	the user typing to an application on a window.
**************************************************************************/
termio_normal()
{
	int	status;

	term_outgo();
	status = ioctl( 1, TCSETAW, &T_normal );
	if ( status < 0 )
	{
		perror( "ioctl TCSETAW normal failed" );
		return( -1 );
	}
	return( 0 );
}
/**************************************************************************
* term_drain
*	Wait for all characters to clear the tty buffers on the real line.
**************************************************************************/
term_drain()
{
	ioctl( 1, TCSBRK, 1 );
}
/**************************************************************************
* termio_normal_master
*	Set the pseudo tty to the default settings corresponding the the
*	way the real tty was set.
**************************************************************************/
termio_normal_master( fd )			/* PSEUDOTTY */
	int	fd;
{
	int	status;

	term_outgo();
					/* not FT_USE_TERMIOS */
	status = ioctl( fd, TCSETA, &T_normal_master );
	if ( status < 0 )
	{
		perror( "termio_normal_master TCSETA" );
		return( -1 );
	}
	status = ioctl( fd, TCFLSH, 0 );
	if ( status < 0 )
	{
		perror( "termio_normal_master TCFLSH" );
		return( -1 );
	}
					/* PSEUDOTTY not SYS5_STREAM_PTYS */
					/* not FT_USE_TERMIOS */
	status = ioctl( fd, TIOCSLTC, &T_ltchars_master );
	if ( status < 0 )
	{
		perror( "termio_normal_master TIOCSLTC" );
		return( -1 );
	}
	return( 0 );
}
/**************************************************************************
* termio_normal_master_off
*	Set the pseudo tty to an appropriate ( non-echo ) mode for the
*	window not being active.
**************************************************************************/
termio_normal_master_off( fd )				/* PSEUDOTTY */
	int	fd;
{
	int	status;

	term_outgo();
	status = ioctl( fd, TCSETA, &T_normal_master_off );
	if ( status < 0 )
	{
		perror( "termio_normal_master_off TCSETA" );
		return( -1 );
	}
	return( 0 );
}
							/* PSEUDOTTY */
/**************************************************************************
* window_termio_hex
*	Return a string in "string" that corresponds to a hex dump of
*	the termio structure.  This is used to set the windows on systems
*	that will not allow TERMIO ioctls on the master side of ptys.
**************************************************************************/
window_termio_in_hex( string )
	char	*string;
{
	char	*p;
	int	i;

	p = (char *) &T_normal_master;
	for ( i = 0; i < sizeof( struct termio ); i++ )
		sprintf( &string[ i * 2 ], "%02x", *p++ & 0x00FF );
}
/**************************************************************************
* window_ltchars_in_hex
*	Return a string in "string" that corresponds to a hex dump of
*	the ltchars structure.  This is used to set the windows on systems
*	that will not allow TERMIO ioctls on the master side of ptys.
**************************************************************************/
window_ltchars_in_hex( string )
	char	*string;
{
	char	*p;
	int	i;

	p = (char *) &T_ltchars_master;
	for ( i = 0; i < sizeof( struct ltchars ); i++ )
		sprintf( &string[ i * 2 ], "%02x", *p++ & 0x00FF );
}
get_tiocwinsz( p_rows, p_cols )
	int	*p_rows;
	int	*p_cols;
{
	return( -1 );
}
