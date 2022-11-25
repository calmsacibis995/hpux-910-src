/**			opt_utils.c			**/

/*
 *  @(#) $Revision: 64.3 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  This file contains routines that might be needed for the various
 *   machines that the mailer can run on.  Please check the Makefile
 *   for more help and/or information. 
 */


#include "headers.h"


#ifdef NEED_CUSERID
# include <pwd.h>
#endif

#ifdef NEED_GETHOSTNAME
#  include <sys/types.h>
#  include <sys/utsname.h>
#endif


#ifdef NEED_GETHOSTNAME


int
gethostname( hostname, size )			/* get name of current host */

	int             size;
	char           *hostname;

{
	/*
	 *  Return the name of the current host machine. 
	 *
	 *  This routine compliments of Scott McGregor at the HP
	 *  Corporate Computing Center 
	 */


	int             uname();
	struct utsname  name;


	(void) uname( &name );
	(void) strncpy( hostname, name.nodename, size - 1 );
	hostname[size - 1] = '\0';

}

#endif


#ifdef NEED_CUSERID

char		buf[10];

char *
cuserid( uname )

	char           *uname;

{
	/*
	 *  Added for compatibility with Bell systems, this is the last-ditch
	 *  attempt to get the users login name, after getlogin() fails.  It
	 *  instantiates "uname" to the name of the user...(it also tries
	 *  to use "getlogin" again, just for luck)
	 */


	struct passwd   *password_entry, 
			*getpwuid();
	char            *name, 
			*getlogin();


	if ( uname == NULL )
		uname = buf;

	if ( (name = getlogin()) != NULL )
		return strcpy( uname, name );
	else if ( (password_entry = getpwuid(groupid)) != NULL )
		return strcpy( uname, password_entry->pw_name );
	else
		return NULL;
}

#endif
