/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: wincommon.c,v 70.2 93/02/19 14:28:30 ssa Exp $ */
/**************************************************************************
* wincommon.c
*	Modules in common between facetterm and facet
**************************************************************************/

#include "ptydefs.h"

#include "wins.h"

#include "facetwin.h"
#include "wincommon.h"
#include <string.h>
#include "ptypath.h"
#include "facetpath.h"

char	*Text_sender_removing_utmp =
"Facet process: sender removing /etc/utmp entries...\n";

						/* NO_SUID_ROOT */
/**************************************************************************
* set_owner_on_windows
* set_owner_on_window
* reset_owner_on_windows
*	Normal ttys are owned by the user and mode 0622 while active
*	and are owned by root and are mode 0666 when inactive.
*	Perform the same functions on the windows using utility programs.
**************************************************************************/
#include "fd.h"
set_owner_on_windows()				/* NO_SUID_ROOT */
{
	int	i;
	char	command[ 1000 ];
	char	buff[ 80 ];

	sprintf( command, "%s/sys/%sroot set_owner",
						Facettermpath, Facetprefix );
	for ( i = 0; i < Wins; i++ )
	{
		sprintf( buff, " %d %s", Fd_master[ i ], Slave_pty_path[ i ] );
		strcat( command, buff );
	}
	tsmsystem( command );
}
set_owner_on_window( winno )			/* NO_SUID_ROOT */
	int	winno;
{
	char	command[ 1000 ];
	char	buff[ 80 ];

	sprintf( command, "%s/sys/%sroot set_owner",
						Facettermpath, Facetprefix );
	sprintf( buff, " %d %s", Fd_master[ winno ], Slave_pty_path[ winno ] );
	strcat( command, buff );
	tsmsystem( command );
}
reset_owner_on_windows()			/* NO_SUID_ROOT */
{
	int	i;
	char	command[ 1000 ];
	char	buff[ 80 ];

	sprintf( command, "%s/sys/%sroot reset_owner",
						Facettermpath, Facetprefix );
	for ( i = 0; i < Wins; i++ )
	{
		sprintf( buff, " %d %s", Fd_master[ i ], Slave_pty_path[ i ] );
		strcat( command, buff );
	}
	tsmsystem( command );
}
						/* NO_SUID_ROOT */
#include <stdio.h>
#include <sys/types.h>

						/* not NO_UTMP */

#include <utmp.h>

/**************************************************************************
* set_utmp_idle
* set_utmp_idle_all
*	Remove the utmp entry for "winno" or for all windows.
**************************************************************************/
set_utmp_idle( winno )				/* NO_SUID_ROOT */
	int	winno;
{
	set_utmp_range( "UTMPIDLE", 0, winno, winno );
}
set_utmp_idle_all()				/* NO_SUID_ROOT */
{
	printf( Text_sender_removing_utmp );
	fflush( stdout );
	set_utmp_range( "UTMPIDLE", 0, 0, Wins - 1 );
}
/**************************************************************************
* set_utmp_range
*	Add /etc/utmp entries for user name "user" and pid "pid" for
*	window numbers "first" to "last".
**************************************************************************/
set_utmp_range( user, pid, first, last )		/* NO_SUID_ROOT */
	char	*user;
	int	pid;
	int	first;
	int	last;
{
	char	buff[ 80 ];
	char	command[ 1024 ];
	int	i;

	sprintf( command, "%s/sys/%sutmp %s %d", Facettermpath, Facetprefix,
					  user, pid );
	for ( i = first; i <= last; i++ )
	{
		sprintf( buff, " %d %s", Fd_master[ i ], Slave_pty_path[ i ] );
		strcat( command, buff );
	}
	tsmsystem( command );
}
						/* NO_SUID_ROOT */
#include "cuserid.h"
/**************************************************************************
* set_utmp_in_use
*	Add a /etc/utmp entry for pid "pid_string" on window number "winno".
**************************************************************************/
set_utmp_in_use( pid_string, winno )		/* NO_SUID_ROOT */
	char	*pid_string;
	int	winno;
{
	int	pid;

	pid = atoi( pid_string );
	set_utmp_range( Cuserid, pid, winno, winno );
}
						/* NO_SUID_ROOT */
						/* not NO_UTMP */
/**************************************************************************
* startwin
*	Runs the command in "string" on the window "window".
**************************************************************************/
#include "ptypath.h"
startwin( window, window_title, string )
	int	window;
	char	*window_title;
	char	*string;
{
	char	command[ STARTWIN_COMMAND_MAX + 
			 STARTWIN_TITLE_MAX +
			 STARTWIN_WINDOW_PATH_MAX + 1 ];

	sprintf( command, "%s/sys/%sstartw %s %d \"", 
			Facettermpath, Facetprefix, Slave_pty_path [ window ],
			window + 1 );
	strncat( command, window_title, STARTWIN_TITLE_MAX );
	strcat(  command, "\" " );
	strncat( command, string, STARTWIN_COMMAND_MAX );
	command[ STARTWIN_COMMAND_MAX + 
		 STARTWIN_TITLE_MAX + 
		 STARTWIN_WINDOW_PATH_MAX ] = '\0';
	mysystem( command );
}
/**************************************************************************
* blank_trim
*	Trim trailing blanks from the string "string".
**************************************************************************/
blank_trim( string )
	char	*string;
{
	int	i;

	for ( i = strlen( string ) - 1; i >= 0; i-- )
	{
		if ( string[ i ] == ' ' )
			string[ i ] = '\0';
		else
			return;
	}
}
