/**			validname.c			**/

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
 *  This contains a routine that checks the given address against the
 *  known local information to ascertain if it could successfully be
 *  delivered locally.  The algorithm is:
 *
 *	1. Is there a file '/usr/mail/%s'?  
 *	2. Is there a password entry for %s?
 *
 */


#include "headers.h"

#ifndef NOCHECK_VALIDNAME


#include <pwd.h>


int
valid_name( name )

	char           *name;

{
	/*
	 *  does what it says above, boss! 
	 */
	 

	struct passwd   *getpwnam();
	char            filebuf[SLEN];


	sprintf( filebuf, "%s/%s", mailhome, name );

	if ( access(filebuf, ACCESS_EXISTS) == 0 )
		return ( 1 );

	if ( getpwnam(name) != NULL )
		return ( 1 );

	return ( 0 );
}

#endif
