/**			returnaddr.c			**/

/*
 *  @(#) $Revision: 70.3 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  This set of routines is used to generate real return addresses
 *  and also return addresses suitable for inclusion in a users
 *  alias files (ie optimized based on the pathalias database).
 *
 *  Added: the ability to respond to messages that were originally
 *  sent by the user (That is, the "savemail" file format messages)
 *  by reading the return address, seeing the "To:" prefix and then
 *  calling the "get_existing_return()" routine.  Currently this does
 *  NOT include any "Cc" lines in the message, just the "To:" line(s).
 *
 *  Also added the PREFER_UUCP stuff for listing reasonable addresses
 *  and such...*sigh*
 */


#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "headers.h"


#ifdef NLS
# define NL_SETN  35
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


#ifdef OPTIMIZE_RETURN


int
optimize_return( address )

	char           *address;

{
	/*
	 *  This routine tries to create an optimized address, that is,
	 *  an address that has the minimal information needed to 
	 *  route a message to this person given the current path
	 *  database...
	 */


#ifdef PREFER_UUCP

	/*
	 *  first off, let's see if we need to strip off the localhost
	 *  address crap...
	 *
	 *  if we have a uucp part (e.g.a!b) AND the bogus address...
	 */

	if ( chloc(address, '!') != -1 && in_string(address, BOGUS_INTERNET) )
		address[strlen(address) - strlen(BOGUS_INTERNET)] = '\0';
#endif

	/*
	 *  next step is to figure out what sort of address we have... 
	 */

	if ( chloc(address, '%') != -1 )
		optimize_cmplx_arpa( address );
	else if ( chloc(address, '@') != -1 )
		optimize_arpa( address );
	else
		optimize_usenet( address );
}


int
optimize_cmplx_arpa( address )

	char           *address;

{
	/*
	 *  Try to optimize a complex ARPA address.  A Complex address is one 
	 *  that contains '%' (deferred '@').  For example:  
	 * 	veeger!hpcnof!hplabs!joe%sytech@syte  
	 *  is a complex address (no kidding, right?).  The algorithm for 
	 *  trying to resolve it is to move all the way to the right, then 
	 *  back up left until the first '!' then from there to the SECOND 
	 *  metacharacter on the right is the name@host address...(in this 
         *  example, it would be "joe%sytech").  Check this in the routing
	 *  table.  If not present, keep backing out to the right until we
	 *  find a host that is present, or we hit the '@' sign.  Once we
	 *  have a 'normal' ARPA address, hand it to optimize_arpa().
	 */


	char            name[SHORT_SLEN], 
			buffer[VERY_LONG_STRING], 
			junk[VERY_LONG_STRING],
	                host[SHORT_SLEN], 
			old_host[SHORT_SLEN];

	register int    i, 
			loc, 
			nloc = 0, 
			hloc = 0, 
			passes = 1;

	/*
	 *  first off, get the name%host... 
	 */

	for ( loc = strlen(address) - 1; address[loc] != '!' && loc > -1; loc-- );

	while ( address[loc] != '\0' ) {

		if ( passes == 1 ) {
			loc++;

			while ( address[loc] != '%' && address[loc] != '@' )
				name[nloc++] = address[loc++];
		} else {
			for ( i = 0; old_host[i] != '\0'; i++ )
				name[nloc++] = old_host[i];
		}

		loc++;

		while ( address[loc] != '%' && address[loc] != '@' )
			host[hloc++] = address[loc++];

		host[hloc] = name[nloc] = '\0';

		strcpy( old_host, host );

		sprintf( buffer, "%s@%s", name, shift_lower(host) );

		if ( expand_site(buffer, junk) == 0 ) {
			strcpy( address, buffer );
			return;
		} else if ( address[loc] == '@' ) {
			optimize_arpa( address );
			return;
		} else
			name[nloc++] = '%';		/* for next pass through */

	}
}


int
optimize_arpa( address )

	char           *address;

{
	/*
	 *  Get an arpa address and simplify it to the minimal
	 *  route needed to get mail to this person...
	 */


	char            name[SHORT_SLEN], 
			buffer[VERY_LONG_STRING], 
			junk[VERY_LONG_STRING],
	                host[SHORT_SLEN];
	register int    loc, 
			nloc = 0, 
			hloc = 0, 
			at_sign = 0;


	for ( loc = strlen(address) - 1; address[loc] != '!' && loc > -1; loc-- ) {
		if ( address[loc] == '@' )
			at_sign++;		/* remember this spot! */
		else if ( at_sign )
			name[nloc++] = address[loc];
		else
			host[hloc++] = address[loc];
	}

	name[nloc] = host[hloc] = '\0';

	reverse( name );
	reverse( host );

	sprintf( buffer, "%s@%s", name, shift_lower(host) );

	if ( expand_site(buffer, junk) == 0 ) {
		strcpy( address, buffer );
		return;
	}

	optimize_usenet( address );		/* that didn't work... */
}


int
optimize_usenet( address )

	char           *address;

{
	/*
	 *  optimize the return address IFF it's a standard usenet
	 *  address...
	 */


	char            name[SHORT_SLEN], 
			new_address[VERY_LONG_STRING], 
			buffer[VERY_LONG_STRING], 
			junk[VERY_LONG_STRING];

	register int    loc, 
			nloc = 0, 
			aloc = 0, 
			passes = 1;


	for ( loc = strlen(address) - 1; address[loc] != '!' && loc > -1; loc-- )
		name[nloc++] = address[loc];

	name[nloc] = '\0';

	reverse( name );

	new_address[0] = '\0';

	/*
	 * got name, now get machine until we can get outta here 
	 */

	while ( loc > -1 ) {

		new_address[aloc++] = address[loc--];	/* the '!' char */

		while ( address[loc] != '!' && loc > -1 )
			new_address[aloc++] = address[loc--];

		new_address[aloc] = '\0';

		strcpy( buffer, new_address );
		reverse( buffer );

		if ( expand_site(buffer, junk) == 0 ) {
			if ( passes == 1 && chloc(name, '@') == -1 ) {
				buffer[strlen(buffer) - 1] = '\0';	/* remove '!' */
				sprintf( address, "%s@%s", name, buffer );
			} else
				sprintf( address, "%s%s", buffer, name );
			return;				/* success! */
		}

		passes++;
	}

	return;						/* nothing to do! */
}

#endif /* OPTIMIZE_RETURN */


int
get_return( buffer, for_reply )

	char           *buffer;
	int		for_reply;

{
	/*
	 *  reads 'current' message again, building up the full return 
	 *  address including all machines that might have forwarded 
	 *  the message.  
	 *  if for_reply is TRUE, searches Reply-To: list, flag is FALSE
	 *  (to use save_by_name), doesn't.
	 */


	char            buf[VERY_LONG_STRING], 
			buf2[VERY_LONG_STRING],
			name1[SLEN], 
			name2[SLEN], 
			lastname[SLEN],
	                hold_return[VERY_LONG_STRING], 
			alt_name1[SLEN],
			alt_name2[SLEN];
	int             ok = 1, 
			lines;
	size_t		len_buf,
			len_buf2;


	buf[0] = name1[0] = name2[0] = lastname[0] = '\0';
	hold_return[0] = alt_name1[0] = alt_name2[0] = '\0';

	/*
	 *  get to the first line of the message desired 
	 */

	if ( fseek(mailfile, header_table[current - 1].offset, 0) == -1 ) {
		dprint( 1, (debugfile,
			"Error: seek %ld bytes into file hit errno %s (%s)",
			header_table[current - 1].offset, error_name(errno),
			"get_return") );
		error2( catgets(nl_fd,NL_SETN,1,"couldn't seek %d bytes into file (%s)"),
		       header_table[current - 1].offset, error_name(errno) );
		return;
	}

	/*
	 *  okay!  Now we're there!  
	 */

	lines = header_table[current - 1].lines;

	buffer[0] = '\0';

	ok = (int)(fgets(buf2, VERY_LONG_STRING, mailfile) != NULL);
	if (ok) {
		len_buf2 = strlen(buf2);
		if (buf2[len_buf2-1] == '\n')
			lines--;
	}
	while ( ok && lines ) {
		buf[0] = '\0';
		strncat(buf, buf2, VERY_LONG_STRING);
		len_buf = strlen(buf);
		ok = (int)(fgets(buf2, VERY_LONG_STRING, mailfile) != NULL);
		if (ok) {
			len_buf2 = strlen(buf2);
			if (buf2[len_buf2-1] == '\n')
				lines--;
		}

		/* assemble any folded header lines into a single string
		   before processing */
		while(ok && lines && whitespace(buf2[0]) && len_buf >= 2) {
			if (buf[len_buf-1] == '\n') {
				len_buf--;
				buf[len_buf] = '\0';
			}
			strncat(buf, buf2, VERY_LONG_STRING-len_buf-1);
			len_buf = strlen(buf);
			ok = (int)(fgets(buf2, VERY_LONG_STRING, mailfile) != NULL);
			if (ok) {
				len_buf2 = strlen(buf2);
				if (buf2[len_buf2-1] == '\n')
					lines--;
			}
		}

		/* buf now contains the unfolded header line to be examined
		   for a return address.  buf2 contains the line of the message
		   that follows this header line. */

		if ( real_from(buf, (struct header_rec *)0) )
			sscanf( buf, "%*s %s", hold_return );
		else if ( first_word(buf, ">From ") ) {
			sscanf( buf, "%*s %s %*s %*s %*s %*s %*s %*s %*s %s %s %s",
			       name1, name2, alt_name1, alt_name2 );
			if ( equal(name2, "from") )	/* remote from xyz  */
				strcpy( name2, alt_name1 );
			else if ( equal(name2, "remote") )/* remote from xyz  */
				strcpy( name2, alt_name2 );
			else if ( equal(name2, "by") )	/* forwarded by xyz */
				strcpy( name2, alt_name1 );
			else if ( equal(name2, "forwarded") )/* forwarded by xyz */
				strcpy( name2, alt_name2 );

			add_site( buffer, name2, lastname );
		}


#ifdef USE_EMBEDDED_ADDRESSES

		else if ( first_word(buf, "From:" ) || first_word(buf, ">From:" )) {
			get_address_from( "From:", buf, hold_return );
			buffer[0] = '\0';
		} else if ( first_word(buf, "Reply-To:") && for_reply ) {
			buffer[0] = '\0';
			get_address_from( "Reply-To:", buf, buffer );
			return;
		}
#endif


		else if ( len_buf < 2 )			/* done with header */
			lines = 0;			/* let's get outta here!  We're
					 	 	 * done!!! */
	}

	if ( buffer[0] == '\0' )
		strcpy( buffer, hold_return );	 	/* default address! */
	else
		add_site( buffer, name1, lastname );	/* get the user name
							 * too! */

	if ( first_word(buffer, "To:") )	 	/* response to savecopy!  */
		get_existing_address( buffer );
	else if ( chloc(header_table[current-1].from, ' ') >= 0 ||
		 (chloc(header_table[current-1].from, '!') <  0 &&
		  chloc(header_table[current-1].from, '@') <  0 )){
		sprintf( name2, " (%s)", header_table[current-1].from );
		strcat( buffer, name2 );
	}
		
}


int
get_existing_address( buffer )

	char           *buffer;

{
	/*
	 *  This routine is called when the message being responded to has
	 *  "To:xyz" as the return address, signifying that this message is
	 *  an automatically saved copy of a message previously sent.  The
	 *  correct to address can be obtained fairly simply by reading the
	 *  To: header from the message itself and (blindly) copying it to
	 *  the given buffer.  Note that this header can be either a normal
	 *  "To:" line (Elm) or "Originally-To:" (previous versions e.g.Msg)
	 */


	char            mybuf[VERY_LONG_STRING],
			temp_buffer1[VERY_LONG_STRING],
			temp_buffer2[VERY_LONG_STRING];
	register char   ok = 1, 
			in_to = 0;


	buffer[0] = '\0';
	temp_buffer1[0] = '\0';
	temp_buffer2[0] = '\0';

	/*
	 *  first off, let's get to the beginning of the message... 
	 */

	if ( fseek(mailfile, header_table[current - 1].offset, 0) == -1 ) {
		dprint( 1, (debugfile,
			"Error: seek %ld bytes into file hit errno %s (%s)",
			header_table[current - 1].offset, error_name(errno),
			   "get_existing_address") );
		error2( catgets(nl_fd,NL_SETN,2,"couldn't seek %d bytes into the file (%s)"),
		       header_table[current - 1].offset, error_name(errno) );
		return;
	}

	/*
	 *  okay!  Now we're there!  
	 */

	while ( ok ) {
		ok = (int) ( fgets(mybuf, VERY_LONG_STRING, mailfile) != NULL );
		no_ret( mybuf );			/* remove return character */

		if ( first_word(mybuf, "To: ") ) {
			in_to = TRUE;
			strcpy( temp_buffer1, mybuf + strlen("To: ") );

		} else if ( first_word(mybuf, "Original-To: ") ) {
			in_to = TRUE;
			strcpy( temp_buffer1, mybuf + strlen("Original-To: ") );

		} else if ( in_to && whitespace(mybuf[0]) ) {
			if ( strlen(temp_buffer1)+strlen(mybuf)
				     < VERY_LONG_STRING ) {
				strcat( temp_buffer1, " " );	/* tag a space in   */
				strcat( temp_buffer1, mybuf + 1 );/* skip 1 whitespace */
			} else
				break;

		} else if ( strlen(mybuf) < 2 ) 
			break;
		else
			in_to = 0;
	}

	if ( strlen( temp_buffer1 ) > VERY_LONG_STRING ) {
		strcpy( temp_buffer2, strip_parens(temp_buffer1) );
		strcpy( temp_buffer1, temp_buffer2 );
		
		if ( strlen(temp_buffer1) > VERY_LONG_STRING )
		       temp_buffer1[VERY_LONG_STRING-1] = '\0';
	}

	strcpy( buffer, temp_buffer1 );

	return;				/* we're done for!  */
}
