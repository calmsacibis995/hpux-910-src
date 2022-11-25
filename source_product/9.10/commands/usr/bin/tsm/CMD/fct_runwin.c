/*****************************************************************************
** Copyright (c) 1986 - 1989 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: fct_runwin.c,v 70.1 92/03/09 15:38:03 ssa Exp $ */
#ifndef lint
static char rcsID[] = "@(#) $Revision: 70.1 $ Copyright (c) 1990 by SSSI";
#endif
/* #define DEBUG 1 */
/*
 * fct_runwin	%W% %U% %G%
 *
 * fct_runwin window_number window_title program_name [ program_arguments ... ]
 *
 * executes "program_name" in window "window_number".
 */

#include	<stdio.h>
#include	<sys/types.h>
#include	<fcntl.h>
/* #include	<termio.h> */
#include	<ctype.h>
#include	"facetterm.h"

#include	<errno.h>
extern int	errno;
extern char	*sys_errlist[];
extern int	sys_nerr;

extern  char	*ttyname();
int	Errchan = 0;
#define WINDOW_DEVICE_MAX 80
char	Window_device[ WINDOW_DEVICE_MAX + 1 ];
char	Buff[ 80 ];
char	Env_window_number[ 19 + 10 ] = "FACETWINDOWNUMBER=";

int	Args_used_here = 3;

main (argc, argv)	/* window number, title, program, arguments */
	int	argc;
	char	*argv[];
{
	int	i;
	char	*progname;
	int	ttyfd;
	int	pid;
	char	*ttyn;
	int	len;
	int	new_window;
	char	*err;
	char	*window_title;
	char	*facet_ttyname;
	char	*get_facet_ttyname();
	char	*facet_ptyname;
	char	*get_facet_ptyname();
	char	window_number_string[ 20 ];
	char	*profile_arg;
	char	*p;
	char	*strdup();
	char	*strrchr();

	     if (argc < 4)
	{
		exit( 100 + EINVAL );
	}
	else
	{
		/**********************************************************
		* Encode the window number specified by the user. 
		* 1 to 10 = specified.  0 = use idle. -1 = error.
		**********************************************************/
		new_window = get_new_window( argv[ 1 ] );
		if ( new_window < 0 )
			exit( 100 + EINVAL );
		/**********************************************************
		* Remember window_title.
		**********************************************************/
		window_title = argv[ 2 ];
	}
	/******************************************************************
	* Stdin must be a tty and a FACET window.
	******************************************************************/
	if ( (ttyn = ttyname(0)) == 0 )
		exit( 100 + ENOTTY );
	facet_ttyname = get_facet_ttyname();
	if ( facet_ttyname == (char *) 0 )
		exit( 100 + ENOTTY );
	/******************************************************************
	* If user specified a window, make sure it is idle.
	* Else choose an idle window.
	******************************************************************/
	if ( new_window )
	{
		if ( check_active_win( new_window ) )
			exit( 0 );
	}
	else
	{
		new_window = get_idle_win();
		if ( new_window < 1 || new_window > 10 )
			exit( new_window );
	}
	/******************************************************************
	* Construct the new window name.
	******************************************************************/
	facet_ptyname = get_facet_ptyname( new_window );
	if ( facet_ptyname == (char *) 0 )
		exit( 100 + ENOTTY );
	strncpy( Window_device, facet_ptyname, WINDOW_DEVICE_MAX );
	Window_device[ WINDOW_DEVICE_MAX ] = '\0';
	/******************************************************************
	* Make sure it exists.
	******************************************************************/
	if ( access( Window_device, 0 ) < 0 )
		exit( 100 + errno );
	/******************************************************************
	* Fork and return window number to parent.
	******************************************************************/
	pid = fork();
	if ( pid < 0)		/* failed */
		exit( 100 + errno );
	if ( pid > 0 )		/* parent */
		exit( new_window );
	/******************************************************************
	* CHILD CODE FOLLOWS.
	******************************************************************/
	/******************************************************************
	* Close all files except parent's stderr dup'ed on file descriptor
	* 4 which is marked close on exec. It may be used for errors.
	******************************************************************/
	for (i = 3; i < 128; i++)
		close(i);
	if ( ( Errchan = dup (2) ) >= 0 )
		fcntl( Errchan, F_SETFD, 1 );
	close (0);
	close (1);
	close (2);
	/******************************************************************
	* fct_ioctl channel was closed as a side effect.
	******************************************************************/
	fct_ioctl_reset();
	/******************************************************************
	* Make the new process the head of a new process group and open the
	* window as stdin, stdout, and stderr.
	******************************************************************/
	setpgrp();
	if (  ( open( Window_device, O_RDWR ) != 0 )
	   || ( dup(0) != 1 ) 
	   || ( dup(0) != 2 ) )
	{
		if ( errno < sys_nerr )
		{
			sprintf( Buff, "\r\nCan not open \"%s\": \"%s\"\r\n",
				Window_device, sys_errlist[ errno ] );
		}
		else
		{
			sprintf( Buff, 
			     "\r\nCan not open \"%s\": Unknown error # %d\r\n",
			     Window_device, errno );
		}
		write( Errchan, Buff, strlen( Buff ) );
		exit( 100 + errno );
	}
	/******************************************************************
	* Subsequent errors go to the window.
	******************************************************************/
	close( Errchan );
	/******************************************************************
	* Some ptys need line discipline pushed - if so do it here.
	******************************************************************/
	set_window_line_discipline();
	/******************************************************************
	* Try to open /dev/tty. 
	* If it will not open, the process has no window group id.
	* This means that there is already a process group leader 
	* on this window. That is, it is already in use.
	******************************************************************/
	if ( (ttyfd = open( "/dev/tty", O_RDWR )) < 0 )
	{
		printf( "\r\n%s is already in use.\r\n", Window_device );
		exit( 100 + errno );
	}
	close( ttyfd );
	/******************************************************************
	* If the initial stty settings are not done by the driver or from
	* the master side of the pty - set them here.
	******************************************************************/
	set_window_stty();
	/******************************************************************
	* Enter this process in /etc/utmp.
	******************************************************************/
	set_utmp_to_active();
	/******************************************************************
	* Export window number
	******************************************************************/
	sprintf( window_number_string, "%d", new_window );
	strcat( Env_window_number, window_number_string );
	putenv( Env_window_number );
	/******************************************************************
	* Adjust args for new process. This program, window name, title.
	******************************************************************/
	argc -= Args_used_here;
	for (i = 0; i < argc; i++)
		argv[ i ] = argv[ i + Args_used_here ];
	argv[i] = (char *) 0;
	/******************************************************************
	* Send title ( or program name and arguments ) to facetterm.
	******************************************************************/
	if ( ( window_title == (char *) 0 ) || ( *window_title == '\0' ) )
		exec_list( argc, argv );
	else
		exec_list( 1, &window_title );
	/******************************************************************
	* Close the fct_ioctl channel if it is open.
	******************************************************************/
	fct_ioctl_close();
	/******************************************************************
	* Run the program.
	******************************************************************/
	progname = argv[0];
	if (*progname == '-' )
	{
		progname++;
		profile_arg = strdup( argv[ 0 ] );
		p = strrchr( profile_arg, '/' );
		if ( p != (char *) 0 )
		{
			argv[ 0 ] = p;
			*argv[ 0 ] = '-';
		}
	}
	execvp (progname, argv);
	/******************************************************************
	* If here, the exec failed.
	******************************************************************/
	fprintf( stderr, "\r\n" );
	perror( argv[ 0 ] );
	fprintf( stderr, "\r\n" );
	exit( 100 + errno );
	/*NOTREACHED*/
}
					/* 1 thru 9 = window #    - return # */
					/* 10       = window 10   - return 10 */
					/* 0        = window 10   - return 10 */
					/* -1       = choose idle - return 0 */
					/* other    = error       - return -1 */
get_new_window( s )
	char	*s;
{
	int	new_window;

	new_window = atoi( s );
	if ( ( new_window >= 1 ) && ( new_window <= 10 ) )
		return( new_window );
	else if ( ( new_window == 0 ) && ( strcmp( s, "0" ) == 0 ) )
		return( 10 );
	else if ( new_window == -1 )
		return( 0 );
	else
		return( -1 );
}

get_idle_win()
{
	char	windows[ FIOC_BUFFER_SIZE ];
	int	idle;

	if ( fct_ioctl( 0, FIOC_IDLE, windows ) == -1 )
		return( 100 + errno );
	idle = atoi( windows );
	return( idle );
}
check_active_win( window )
	int	window;			/* 1 to 10 */
{
	char	windows[ FIOC_BUFFER_SIZE ];
	char	*p;
	char	c;
	int	active;

	if ( fct_ioctl( 0, FIOC_ACTIVE, windows ) == -1 )
		return( 100 + errno );
	p = windows;
	while ( *p != '\0' )
	{
		active = atoi( p );
		if ( window == active )
			return( 1 );		/* active */
		while ( *p != '\0' )
		{
			if ( *p++ == ' ' )
				break;
		}
	}
	return( 0 );				/* not active */
}
char	Get_facet_ptyname[ FIOC_BUFFER_SIZE ];

char	*
get_facet_ptyname( window )
	int	window;			/* 1 to 10 */
{
	char	*answer;

	sprintf( Get_facet_ptyname, "ptyname_%d", window );
	if ( fct_ioctl( 0, FIOC_GET_INFORMATION, Get_facet_ptyname ) < 0 )
		answer = (char *) 0;
	else if ( Get_facet_ptyname[ 0 ] == '\0' )
		answer = (char *) 0;
	else
		answer = Get_facet_ptyname;
	return( answer );
}
char	Get_facet_ttyname[ FIOC_BUFFER_SIZE ];

char	*
get_facet_ttyname( ttyname, max )
	char	*ttyname;
	int	max;
{
	char	*answer;

	strcpy( Get_facet_ttyname, "ttyname" );
	if ( fct_ioctl( 0, FIOC_GET_INFORMATION, Get_facet_ttyname ) < 0 )
		answer = (char *) 0;
	else if ( Get_facet_ttyname[ 0 ] == '\0' )
		answer = (char *) 0;
	else
		answer = Get_facet_ttyname;
	return( answer );
}
set_utmp_to_active()
{
	char	buffer[ FIOC_BUFFER_SIZE ];
	int	status;

	sprintf( buffer, "%d", getpid() );
	status = fct_ioctl( 0, FIOC_UTMP, buffer );
	return( status );
}
