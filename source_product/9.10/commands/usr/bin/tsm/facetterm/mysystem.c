#include <signal.h>
mysystem( command )
	char	*command;
{
	int	pid;

	pid = fork();
	if ( pid < 0 )
	{
		term_outgo();
		perror( "fork" );
		term_outgo();
		return( -1 );
	}
	else if ( pid > 0 )
	{
		int	stat_loc;
		int	pid_child;
		void    (*sig2)();

		sig2 = signal( SIGINT, SIG_IGN );
		while( (pid_child = wait( &stat_loc )) > 0 )
		{
			if ( pid == pid_child )
				break;
		}
		signal( SIGINT, sig2 );
		if ( pid_child < 0 )
			return( -1 );
		return( stat_loc );
	}
	else
	{
		int	status;
		int	fd;

		for ( fd = 3; fd < 128; fd ++ )
			close( fd );
#ifdef EXECS_WITH_EUID
		setuid( getuid() );
		setgid( getgid() );
#endif
#ifndef USE_SYSTEM_CALL
		signal( SIGINT, SIG_DFL );
		execl( "/bin/sh", "sh", "-c", command , (char *) 0 );
		exit( 127 );
#else
		status = system( command );
		exit( status );
#endif
	}
}
tsmsystem( command )
	char	*command;
{
	int	pid;

	pid = fork();
	if ( pid < 0 )
	{
		term_outgo();
		perror( "fork" );
		term_outgo();
		return( -1 );
	}
	else if ( pid > 0 )
	{
		int	stat_loc;
		int	pid_child;
		void    (*sig2)();

		sig2 = signal( SIGINT, SIG_IGN );
		while( (pid_child = wait( &stat_loc )) > 0 )
		{
			if ( pid == pid_child )
				break;
		}
		signal( SIGINT, sig2 );
		if ( pid_child < 0 )
			return( -1 );
		return( stat_loc );
	}
	else
	{
		signal( SIGINT, SIG_DFL );
		execl( "/bin/sh", "sh", "-c", command , (char *) 0 );
		exit( 127 );
	}
}
