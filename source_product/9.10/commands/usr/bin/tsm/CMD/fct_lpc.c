#include <stdio.h>
#define MAX_NAME 100
main( argc, argv )
	int	argc;
	char	**argv;
{
	char	command[ 1000 ];

	if ( argc > 2 )
	{
		if (  (strcmp( argv[ 1 ], "abort" ) != 0 )
		   && (strcmp( argv[ 1 ], "start" ) != 0 )
		   )
		{
			exit( 1 );
		}
		if ( strncmp( argv[ 2 ], "fct_tty", 7 ) != 0 )
			exit( 1 );
		sprintf( command, "/usr/etc/lpc %s %s", argv[ 1 ], argv[ 2 ] ); 
		setuid( 0 );
		system( command );
	}
}
