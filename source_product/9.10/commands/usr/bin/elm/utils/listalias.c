/**			listalias.c			**/

/*
 *  @(#) $Revision: 66.1 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  Program that lists all the available aliases.  This one uses the pipe 
 *  command, feeding the stuff to egrep then sort, or just sort.
 *
 *  Modified to never list the SIZE_INDICATOR alias, since that's an
 *  internal `secret' and shouldn't be something that the user ever
 *  sees...
 */


#include <stdio.h>
#include <fcntl.h>

#include "defs.h"


#ifdef NLS
# include <nl_types.h>
# define NL_SETN   3
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


char           *getenv();


#ifdef NLS
extern nl_catd		nl_fd;	/* message catalogue file    */
#endif NLS


int
listalias( args, expression )

	int		args;
	char            *expression;

{
	char		user_hash[SLEN];


	sprintf( user_hash, "%s/%s", getenv("HOME"), ALIAS_HASH );
	if ( access(user_hash, 00) != -1 ) {
		printf( catgets(nl_fd,NL_SETN,5,"User aliases:\n") );
		list_alias( args, expression, FALSE );
		printf("\n");
        }

	if ( access(system_hash_file, 00) != -1 ) {
		printf( catgets(nl_fd,NL_SETN,6,"System aliases:\n") );
		list_alias( args, expression, TRUE );
		printf("\n");
	}

	exit( 0 );
}



int
list_alias( args, expression, system )

	int		args,
			system;
	char		*expression;
{
	FILE            *datafile, 
			*fd_pipe;
	struct alias_rec hash_record;
	int             hashfile;
	char		fd_hash[SLEN], 
			fd_data[SLEN], 
			buffer[VERY_LONG_STRING];

	if ( args > 2 )
		sprintf( buffer, "egrep \"%s\" | sort", expression );
	else
		sprintf( buffer, "sort" );

	if ( system ) {
		strcpy( fd_hash, system_hash_file );
		strcpy( fd_data, system_data_file );
	} else {
		sprintf( fd_hash, "%s/%s", getenv("HOME"), ALIAS_HASH );
		sprintf( fd_data, "%s/%s", getenv("HOME"), ALIAS_DATA );
	}

	if ( (fd_pipe = popen(buffer, "w")) == NULL ) {
		if ( args > 1 )
			printf( catgets(nl_fd,NL_SETN,1,"cannot open pipe to egrep program for expressions!\n") );
		fd_pipe = stdout;
	}

	if ( (hashfile = open(fd_hash, O_RDONLY)) > 0 ) {
		if ( (datafile = fopen(fd_data, "r")) == NULL ) {
			printf( catgets(nl_fd,NL_SETN,2,"Opened %s hash file, but couldn't open data file!\n"),
			        system ? catgets(nl_fd,NL_SETN,3,"system"):catgets(nl_fd,NL_SETN,4,"user") );
			pclose( fd_pipe );
			return;
		}

		/*
		 *  Otherwise let us continue... 
		 */

		while ( read(hashfile, &hash_record, sizeof(hash_record)) != 0 ) {
			if ( strlen(hash_record.name) > 0 &&
			     !equal(hash_record.name, SIZE_INDICATOR) ) {
				fseek( datafile, hash_record.byte, 0L );
				fgets( buffer, VERY_LONG_STRING, datafile );
				fprintf( fd_pipe, "\t%-15s  %s", hash_record.name, 
							       buffer );
			}
		}
	}
	pclose( fd_pipe );
	return;
}
