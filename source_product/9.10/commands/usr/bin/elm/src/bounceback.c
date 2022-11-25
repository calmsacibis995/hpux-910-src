/**			bounceback.c			**/

/*
 *  @(#) $Revision: 70.2 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  This set of routines implement the bounceback feature of the mailer.
 *  This feature allows mail greater than 'n' hops away (n specified by
 *  the user) to have a 'cc' to the user through the remote machine.  
 *
 *  Due to the vagaries of the Internet addressing (uucp -> internet -> uucp)
 *  this will NOT generate bounceback copies with mail to an internet host!
 */

#include "headers.h"

#ifdef BOUNCEBACK_ENABLED

static char	bounce_address[VERY_LONG_STRING];

int
uucp_hops( to )

	char           *to;

{
	/*
	 *  Given the entire "To:" list, return the number of hops in the
	 *  first address (a hop = a '!') or ZERO iff the address is to a
  	 *  non uucp address.
	 */


	register int    hopcount = 0, 
			index;


	for ( index = 0; !whitespace(to[index]) && to[index] != '\0'; index++ ) {
		if ( to[index] == '!' )
			hopcount++;
		else if ( to[index] == '@' || to[index] == '%' || to[index] == ':' )
			return ( 0 );		/* don't continue! */
	}

	return ( hopcount );
}


char 
*bounce_off_remote( to )

	char           *to;

{
	/*
	 *  Return an address suitable for framing (no, that's not it...)
	 *  Er, suitable for including in a 'cc' line so that it ends up
	 *  with the bounceback address.  The method is to take the first 
	 *  address in the To: entry and break it into machines, then 
	 *  build a message up from that.  For example, consider the
	 *  following address:
	 *		a!b!c!d!e!joe
	 *  the bounceback address would be;
	 *		a!b!c!d!e!d!c!b!a!ourmachine!ourname
	 *  simple, eh?
	 */


	char            host[MAX_HOPS][LONG_SLEN];	/* for breaking up addr */
	register int    hostcount = 0, 
			hindex = 0, 
			index;


	for ( index = 0; !whitespace(to[index]) && to[index] != '\0'; index++ ) {
		if ( to[index] == '!' ) {
			host[hostcount][hindex] = '\0';
			hostcount++;
			hindex = 0;
		} else
			host[hostcount][hindex++] = to[index];
	}

	/*
	 * we have hostcount hosts... 
	 */

	strcpy( bounce_address, host[0] );			/* initialize it! */

	for ( index = 1; index < hostcount; index++ ) {
		strcat( bounce_address, "!" );
		strcat( bounce_address, host[index] );
	}

	/*
	 * and now the same thing backwards...
	 */

	for ( index = hostcount - 2; index > -1; index-- ) {
		strcat( bounce_address, "!" );
		strcat( bounce_address, host[index] );
	}

	/*
	 * and finally, let's tack on our machine and login name 
	 */

	strcat( bounce_address, "!" );
	strcat( bounce_address, hostname );
	strcat( bounce_address, "!" );
	strcat( bounce_address, username );

	/*
	 * and we're done!! 
	 */

	return ( (char *) bounce_address );
}

#endif
