/*****************************************************************************
** Copyright (c) 1990        Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: fct_root.c,v 70.1 92/03/09 15:37:58 ssa Exp $ */
#ifndef lint
static char rcsID[] = "@(#) $Revision: 70.1 $ Copyright (c) 1990 by SSSI";
#endif
/*****************************************************************************
** fct_root - perform set uid root functions for facetterm.
*****************************************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#define	SET_OWNER	1
#define RESET_OWNER	2
main( argc, argv )
	int	argc;
	char	**argv;
{
	int		i;
	int		master_pty_fd;
	struct stat	master_pty_name_stat;
	struct stat	master_pty_fd_stat;
	char		*command_string;
	int		command;
	int		realuid;
	int		realgid;
	int		rootuid;
	int		rootgid;
	char		*slave_pty_name;
	char		*slave_to_master();
	char		*master_pty_name;

	/******************************************************************
	** Check that arguments are:
	**	command
	**	pairs of master pty file descriptor, slave pty name
	******************************************************************/
	if ( argc < 4 )
		exit( 1 );
	command_string = argv[ 1 ];
	if ( strcmp( command_string, "set_owner" ) == 0 )
		command = SET_OWNER;
	else if ( strcmp( command_string, "reset_owner" ) == 0 )
		command = RESET_OWNER;
	else
		exit( 1 );
	if ( ( ( argc - 2 ) % 2 ) != 0 )
		exit( 1 );
	/******************************************************************
	** Determine real and effective uid of caller.
	******************************************************************/
	realuid = getuid();
	realgid = getgid();
	rootuid = 0;
	rootgid = 1;
	/******************************************************************
	** Work on pairs of master pty fd and slave pty name.
	******************************************************************/
	for ( i = 2; i < argc; i += 2 )
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
		switch( command )
		{
			case SET_OWNER:
				set_slave_permissions( slave_pty_name,
						       realuid, realgid, 0622);
				break;
			case RESET_OWNER:
				set_slave_permissions( slave_pty_name,
						       rootuid, rootgid, 0666);
				break;
			default:
				exit( 4 );
		}
	}
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
set_slave_permissions( slave_pty_name, uid, gid, permission )
	char	*slave_pty_name;
	int	uid;
	int	gid;
	int	permission;
{
	chown( slave_pty_name, uid, gid);
	chmod( slave_pty_name, permission);
}
