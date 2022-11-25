/*****************************************************************************
** Copyright (c) 1990        Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: fct_utmp.c,v 70.1 92/03/09 15:38:14 ssa Exp $ */
#ifndef lint
static char rcsID[] = "@(#) $Revision: 70.1 $ Copyright (c) 1990 by SSSI";
#endif
/*****************************************************************************
** fct_utmp - perform set uid root /etc/utmp functions for facetterm.
*****************************************************************************/
#include <stdio.h>
#include <sys/types.h>
#include <utmp.h>
#include <sys/stat.h>
#include <sys/utsname.h>
main( argc, argv )
	int	argc;
	char	**argv;
{
	int		i;
	int		master_pty_fd;
	struct stat	master_pty_name_stat;
	struct stat	master_pty_fd_stat;
	char		*ut_user;
	int		command_is_idle;
	char		*slave_pty_name;
	char		*slave_to_master();
	char		*master_pty_name;
	int		ut_pid;

	struct utsname	name;
	int		rev6;
	char		*p;

	if ( uname( &name ) < 0 )
	{
		exit( 5 );
	}
	p = name.release;
	rev6 = 0;
	while ( *p != '\0' )
	{
		if ( ( *p == '6' ) && ( *( p + 1 ) == '.' ) )
		{
			rev6 = 1;
			break;
		}
		p++;
	}
#if ( ! defined( HPREV ) ) || ( HPREV != 6 )
	if ( rev6 )
		exit( 7 );
#else
	if ( rev6 == 0 )
		exit( 7 );
#endif
	/******************************************************************
	** Check that arguments are:
	**	command pid
	**	pairs of master pty file descriptor, slave pty name
	******************************************************************/
	if ( argc < 5 )
		exit( 1 );
	ut_user = argv[ 1 ];
	if ( strcmp( ut_user, "UTMPIDLE" ) == 0 )
	{
		command_is_idle = 1;
	}
	else 
	{
		command_is_idle = 0;
		ut_pid = atoi( argv[ 2 ] );
		if ( ut_pid <= 0 )
			exit( 1 );
	}
	if ( ( ( argc - 3 ) % 2 ) != 0 )
		exit( 1 );
	/******************************************************************
	** Work on pairs of master pty fd and slave pty name.
	******************************************************************/
	for ( i = 3; i < argc; i += 2 )
	{
		/**********************************************************
		** get file number on which master is open.
		**********************************************************/
		master_pty_fd = atoi( argv[ i ] );
		if ( master_pty_fd <= 0 )
			exit( 1 );
		/**********************************************************
		** get slave name and determine master pty name.
		**********************************************************/
		slave_pty_name = argv[ i + 1 ];
		if ( slave_pty_name == (char *) 0 )
			exit( 1 );
		master_pty_name = slave_to_master( slave_pty_name );
		if ( master_pty_name == (char *) 0 )
			exit( 3 );
		/**********************************************************
		** stat the file descriptor and master pty name
		** and insure that they are the same and character device.
		**********************************************************/
		if ( fstat( master_pty_fd, &master_pty_fd_stat ) < 0 )
			exit( 2 );
		if ( stat( master_pty_name, &master_pty_name_stat ) < 0 )
			exit( 2 );
		if ( master_pty_fd_stat.st_dev != master_pty_name_stat.st_dev )
			exit( 3 );
		if ( master_pty_fd_stat.st_ino != master_pty_name_stat.st_ino )
			exit( 3 );
		if ( master_pty_fd_stat.st_mode != master_pty_name_stat.st_mode)
			exit( 3 );
		if ( ( master_pty_fd_stat.st_mode & S_IFMT ) != S_IFCHR )
			exit( 3 );
		/**********************************************************
		** Perform the requested actions on the slave pty.
		**********************************************************/
		if ( command_is_idle )
		{
			set_utmp_idle( slave_pty_name );
		}
		else
		{
			set_utmp_active( slave_pty_name, ut_user, ut_pid );
		}
	}
	exit( 0 );
}
/******************************************************************
** Return the master pty name associated with the slave pty name.
******************************************************************/
	/* slave	/dev/ttyp3	/dev/pty/ttyp3 */
	/* master	/dev/ptyp3	/dev/ptym/ptyp3 */
char	Master_pty_name[ 100 ];

char	*
slave_to_master( slave_pty_name )
	char	*slave_pty_name;	
{
	char	*p;
	char	*result;

	if ( slave_pty_name == (char *) 0 )
		return( (char *) 0 );
	/******************************************************************
	** Slave must start: /dev		Master is the same.
	******************************************************************/
	p = slave_pty_name;
	if ( strncmp( p, "/dev/", 5 ) != 0 )
		return( (char *) 0 );
	strcpy( Master_pty_name, "/dev/" );
	p += 5;
	/******************************************************************
	** If slave is in pty subdirectory -	Master is in ptym subdirectory.
	******************************************************************/
	if ( strncmp( p, "pty/", 4 ) == 0 )
	{
		strcat( Master_pty_name, "ptym/" );
		p += 4;
	}
	/******************************************************************
	** Slave must start: tty    		Master will start: pty
	******************************************************************/
	if ( strncmp( p, "tty", 3 ) != 0 )
		return( (char *) 0 );
	strcat( Master_pty_name, "pty" );
	p +=3;
	/******************************************************************
	** Slave will end: [p-z][0-9a-f]	Master is the same.
	******************************************************************/
	strcat( Master_pty_name, p );
	return( Master_pty_name );
}
set_utmp_idle( slave_pty_name )
	char	*slave_pty_name;
{
	set_window_idle( &slave_pty_name[ 5 ] );
}
set_window_idle( ut_line )
	char		*ut_line;
{
	long		time();
	struct utmp	myutmp;
	struct utmp	*utmp;
	struct utmp	*getutline();  /* line that matches ut_line
					  set LOGIN_PROCESS or USER_PROCESS */
	char		*p;
	int		max_ut_line;
	int		len;

	/*****************************************
	* use the end of the name if it is too long 
	******************************************/
	p = ut_line;
	max_ut_line = sizeof( myutmp.ut_line ) - 1;
	len = strlen( ut_line );
	if ( len > max_ut_line )
		p += ( len - max_ut_line );
	strcpy( myutmp.ut_line, p );
	/*****************************************
	* locate the correct entry - exit 1 on failure
	******************************************/
	setutent();
	utmp = getutline( &myutmp);
	if ( utmp == (struct utmp *) 0 )
		return( -1 );
	/*****************************************
	* set to idle and write back to file - return successfule
	******************************************/
	utmp->ut_type = DEAD_PROCESS;
	utmp->ut_time = time( (long *) 0);
	utmp->ut_exit.e_exit = 0;
	pututline( utmp );
	return( 0 );
}
set_utmp_active( slave_pty_name, ut_user, ut_pid )
	char	*slave_pty_name;
	char	*ut_user;
	int	ut_pid;
{
	set_ut_line_active( &slave_pty_name[ 5 ], ut_user, ut_pid );
}
set_ut_line_active( ut_line, ut_user, ut_pid )
	char	*ut_line;
	char	*ut_user;
	int	ut_pid;
{
	struct utmp	myutmp;
	int		pid;
	char		window_name[ 20 ];
	long		time();
	int		i;

	setutent();
	for ( i = 0; i < sizeof( myutmp ); i++ )
		( (char *) &myutmp )[ i ] = '\0';
	strcpy( myutmp.ut_user, ut_user );
	names( ut_line, myutmp.ut_id, myutmp.ut_line );
	null_fill( myutmp.ut_line, sizeof myutmp.ut_line );
	myutmp.ut_pid = ut_pid;
	myutmp.ut_type = USER_PROCESS;
	myutmp.ut_exit.e_termination = 0;
	myutmp.ut_exit.e_exit = 2;		/* per hpterm */
	myutmp.ut_time = time( (long *) 0 );
	pututline( &myutmp );
}
names( ut_line, id, line )
	char	*ut_line;
	char	*id;
	char	*line;
{
	char	*p;
	char	*d;
	int		max_ut_line;
	struct utmp	myutmp;			/* for size only */

						/* get filename of window */

	/******************************************************************
	* Shorten line if necessary.
	******************************************************************/
	max_ut_line = sizeof( myutmp.ut_line ) - 1;
	p = ut_line;
	while ( strlen( p ) > max_ut_line )
		p++;
	strcpy( line, p );
	/******************************************************************
	* Construct id.
	******************************************************************/
	p = &ut_line[ strlen( ut_line ) - 1 ];
							/* PSEUDOTTY */
							/* HP9000 */
	d = &id[ 3 ];
	/******************************************************************
	** Use the last two or three letters of device for id. Null pad.
	******************************************************************/
	if ( strncmp( p - 6, "tty", 3 ) == 0 )		/* /dev/pty/ttyc000 */
	{
		*d-- = '\0';
		*d-- = *p--;
		*d-- = *p--;
		*d-- = *p--;
	}
	else if ( strncmp( p - 5, "tty", 3 ) == 0 )	/* /dev/pty/ttyc00 */
	{
		*d-- = '\0';
		*d-- = *p--;
		*d-- = *p--;
		*d-- = *p--;
	}
	else
	{
		*d-- = '\0';
		*d-- = '\0';
		*d-- = *p--;
		*d-- = *p--;
	}
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
