/**			msdos.c				**/

/*
 *  @(#) $Revision: 64.2 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  various stubs of routines, added just to help the portability of
 *  the Unix code onto the PC.
 */


#include "headers.h"


#ifdef MSDOS


FILE *popen();
char *gethostname(), *getlogin();

char *
gethostname( hostname, size )

	char 		*hostname;
 	int 		size;

{

	char *cp;


	if ( (cp = getenv("HOSTNAME")) == NULL )
	  	cp = "unknown";
     	  
	strcpy( hostname, cp );

	return( (char *) hostname );
}


pclose( fd )
	
	FILE 		*fd;

{
	printf( "pclose() called, but not available!\n" );
}


FILE *
popen(filename, mode)

	char 		*filename, 
			*mode;

{
	printf( "popen(%s) called, but not available!\n", filename );

	return( (FILE *) NULL );
}


setuid( userid )

	int 		userid;

{
	printf( "setuid(%d) called, but not available!\n", userid );
}


setgid( groupid )

	int 		groupid;

{
	printf( "setgid(%d) called, but not available!\n", groupid );
}


sleep( time )

	int 		time;

{
	register int i;


	for ( i=0; i < time*1000; i++ )
	   ;
}


wait()

{
	printf( "wait() called, but not available!\n" );
}


link( source, dest )

	char 		*source, 
			*dest;

{
	/*
	 *  since file links aren't available, we'll simply
	 *  copy one to the other... 
	 */

	
	FILE 		*sfd, 
			*dfd;
	char 		buffer[SLEN];


	if ( (sfd = fopen(source, "r")) == NULL ) 
		return( -1 );

	if ( (dfd = fopen(dest, "r")) != NULL ) {

	  fclose( dfd );
	  return( -1 );
	}

	if ( (dfd = fopen(dest, "w")) == NULL ) 
		return( -1 );

	while ( fgets(buffer, SLEN, sfd) != NULL ) 
	  	fputs( buffer, dfd );

	fclose( sfd );
	fclose( dfd );

	return( 0 );
}


char *
getlogin()

{
	static char 	*cp;

	if ( (cp = getenv("USERNAME")) == NULL )
	  	return( (char *) "user" );
	else
   	  	return( cp );	
}


int
getgid()

{
	return ( 1 );
}


int
getuid()
{
	return( 1 );
}


chown( filename, owner )

	char 		*filename;
	int 		  owner;

{
	printf( "chown(%s) called, but not available!\n", filename );
}


alarm( _time )

	unsigned 	long _time;

{
	printf( "alarm(%ld) called, but not available!\n", _time );
}


cuserid( name )

	char		 *name;

{
	printf( "cuserid(%s) called, but not available!\n", name );
}


fork()
{
	printf( "fork called, but not available -- returning 0" );

	return( 0 );
}

#endif
