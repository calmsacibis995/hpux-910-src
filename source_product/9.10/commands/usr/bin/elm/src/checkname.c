/**			checkname.c			**/

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
 *  This contains routines to support having a user directory 
 *  called with an arbitrary string, and the return string 
 *  being treated as if the user had actually typed that in 
 *  (a possible return value is *  NULL too).  A simple username
 *  server is located in ../NameServer for your edification.
 */


#include "headers.h"


#ifdef ENABLE_NAMESERVER

#include <signal.h>


int             screen_changed;

void            _exit();


int
check_nameserver( name )

	char           *name;

{
	/*
	 *  If the user has specified an external agent to pass
	 *  the address through, and it hasn't been expanded yet,
	 *  let's do it!  Returns 1 iff it substituted something.
	 *  The global int screen_changed will be set to TRUE iff
	 *  the called nameserver reports that it has changed the
	 *  screen in any way (this is done by the address having 
	 *  a '+' as the first character iff the screen has 
	 *  changed ... )
	 */


	register int    (*istat) (), 
			(*qstat) ();

	char            buffer[SLEN], 
  			command_buffer[SLEN];
	int             results[2];


	if ( nameserver[0] == '\0' )		/* no nameserver specified! */
		return ( 0 );

	buffer[0] = '\0';

	pipe( results );			/* get two file descriptors, one for 
						 * reading and one for writing  */

	sprintf( command_buffer, "%s \"%s\" %d", nameserver,
		 name, results[1] );

	if ( has_transmit )
		transmit_functions( OFF );

	Raw( OFF );

	dprint( 2, (debugfile,
		   "Name Server Call: '%s'\n", command_buffer) );


#ifdef NO_VM					/* machine without virtual memory! */
	if ( fork() == 0 )
#else
	if ( vfork() == 0 )
#endif


	{
		setgid( groupid );		/* and group id		    */
		setuid( userid );		/* back to the normal user! */

		close( results[0] );		/* child doesn't want this one.. */

		(void) signal( SIGHUP, SIG_IGN );	/* kids should ignore this */


		execl( "/bin/sh", "sh", "-c", command_buffer, (char *) 0 );

		_exit(127);
	}

	istat = signal( SIGINT, SIG_IGN );
	qstat = signal( SIGQUIT, SIG_IGN );

	close( results[1] );			/* parent doesn't want this one.. */

	/*
	 * instead of waiting for the exit status of the child, we'll just
	 * wait until the read completes or results in EOF  
	 */

	read( results[0], buffer, SLEN );

	dprint( 2, (debugfile, "server returned: '%s'\n", buffer) );

	close( results[0] );

	signal( SIGINT, istat );
	signal( SIGQUIT, qstat );

	Raw( ON );

	if ( has_transmit )
		transmit_functions( ON );

	if ( buffer[0] == '+' ) {
		screen_changed = TRUE;
		strcpy( name, (char *) buffer + 1 );
	} else
		strcpy( name, buffer );

	return ( buffer[1] == '\0' ? 0 : 1 );
}

#endif
