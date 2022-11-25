/*****************************************************************************
** Copyright (c) 1986 - 1989 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: fct_startw.c,v 70.1 92/03/09 15:38:08 ssa Exp $ */
#ifndef lint
static char rcsID[] = "@(#) $Revision: 70.1 $ Copyright (c) 1990 by SSSI";
#endif
/* #define DEBUG 1 */
/*
 * fct_startwin	%W% %U% %G%
 *
 * executes a program in a window.
 */

#include	<stdio.h>
#include	<sys/types.h>
#include	<fcntl.h>
/* #include	<termio.h> */
#include	<ctype.h>
#include        "facetwin.h"

#include	<errno.h>
extern int	errno;
extern char	*sys_errlist[];
extern int	sys_nerr;

int	Errchan = 0;
char	*Window_device;
char	Buff[ 80 ];
char	*Window_number_ptr;
char	Env_window_number[ 19 + 10 ] = "FACETWINDOWNUMBER=";

#define ARGS_USED_HERE 4

main (argc, argv)	/* device, window_number, title, program, arguments */
	int	argc;
	char	*argv[];
{
	int	i;
	int	realuid, realgid;
	char	*progname;
	int	ttyfd;
	int	pid;
	char	*window_title;
	char	*profile_arg;
	char	*p;
	char	*strdup();
	char	*strrchr();

	if (argc < ARGS_USED_HERE + 1 )
	{
		usage();
		exit(0);
	}

	Window_device = argv[ 1 ];
	Window_number_ptr = argv[ 2 ];
	/******************************************************************
	* Remember window_title.
	******************************************************************/
	window_title = argv[ 3 ];
	/******************************************************************
	* Fork and return to parent.
	******************************************************************/
	pid = fork();
	if ( pid != 0 )			/* parent */
	{
		exit(0);
	}
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
	strcat( Env_window_number, Window_number_ptr );
	putenv( Env_window_number );
	/******************************************************************
	* Renounce SUID root - back to caller's permissions for the exec.
	******************************************************************/
	realuid = getuid();
	realgid = getgid();
	setuid (realuid);
	setgid (realgid);
	/******************************************************************
	* Adjust args for new process. This program, window name, title.
	******************************************************************/
	argc -= ARGS_USED_HERE;
	for (i = 0; i < argc; i++)
		argv[ i ] = argv[ i + ARGS_USED_HERE ];
	argv[i] = (char *) 0;
	/******************************************************************
	* Send title ( or program name and arguments ) to facetterm.
	******************************************************************/
	if ( ( window_title == (char *) 0 ) || ( *window_title == '\0' ) )
		exec_list( argc, argv );
	else
		exec_list( 1, &window_title );
	/******************************************************************
	* Close fct_ioctl channel if being used.
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

usage()
{
static char usage1[] = "Usage:\r\n";
static char usage2[] =
	"\tfct_startwin windowdevice window_number title programname programargs\r\n\n";
static char usage3[] =
	"\twhere 'windowdevice' is the device name of the window in which \r\n";
static char usage4[] =
	"\tto run the program named 'programname'. Following the program\r\n";
static char usage5[] =
	"\tname give any arguments required by the program.\r\n";

	fprintf( stderr, usage1 );
	fprintf( stderr, usage2 );
	fprintf( stderr, usage3 );
	fprintf( stderr, usage4 );
	fprintf( stderr, usage5 );
}
#include	"facetterm.h"
set_utmp_to_active()
{
	char	buffer[ FIOC_BUFFER_SIZE ];
	int	status;

	sprintf( buffer, "%d", getpid() );
	status = fct_ioctl( 0, FIOC_UTMP, buffer );
	return( status );
}
