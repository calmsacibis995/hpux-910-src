/*****************************************************************************
** Copyright (c) 1986 - 1989 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: new_utmp.c,v 70.1 92/03/09 15:38:36 ssa Exp $ */

/* #define DEBUG 1 */
/*
 * new_utmp	%W% %U% %G%
 *
 * makes an entry in /etc/utmp for new process running on a window.
 * facet or facetterm sets it to DEAD_PROCESS when window is idle.
 */


#include	<sys/types.h>
#include	<stdio.h>
#include	 <utmp.h>

void	pututline();
void	utmpname();
#include <string.h>

new_utmp( device )
	char	*device;
{
	struct utmp	myutmp;
	char		*getenv();
	char		*p_cuserid;
	int		pid;
	char		window_name[ 20 ];
	long		time();

	if ( (p_cuserid = getenv( "FACETUSER" )) == NULL )
		return;
	strcpy( myutmp.ut_user, p_cuserid );
	names( device, myutmp.ut_id, myutmp.ut_line );
	null_fill( myutmp.ut_line, sizeof myutmp.ut_line );
	myutmp.ut_pid = getpid();
	myutmp.ut_type = USER_PROCESS;
	myutmp.ut_exit.e_termination = 0;
	myutmp.ut_exit.e_exit = 0;
	myutmp.ut_time = time( (long *) 0 );
	pututline( &myutmp );
	utmpname( "" );
}
names( device, id, line )
	char	*device;
	char	*id;
	char	*line;
{
	char	*p;
	char	*d;
	int		max_ut_line;
	struct utmp	myutmp;			/* for size only */

						/* get filename of window */
	p = strrchr( device, '/' );
	if ( p == NULL )
		p = device;
	else
		p++;
	p = &device[ 5 ];

	max_ut_line = sizeof( myutmp.ut_line ) - 1;
	while ( strlen( p ) > max_ut_line )
		p++;
	strcpy( line, p );

	p = &device[ strlen( device ) - 1 ];
							/* PSEUDOTTY */
							/* HP9000 */
	d = &id[ 3 ];
	/******************************************************************
	** Use the last two letters of device for id. Null pad.
	******************************************************************/
	*d-- = '\0';
	*d-- = '\0';
	*d-- = *p--;
	*d-- = *p--;
}
null_fill( string, len )
	char	*string;
	int	len;
{
	int	i;

	for ( i = 0; i < len; i++ )
	{
		if ( *string++ == '\0' )
		{
			i++;
			break;
		}
	}
	for ( ; i < len; i++ )
		*string++ = '\0';
}
