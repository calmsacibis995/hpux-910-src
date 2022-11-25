/**			reply.c				**/

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
 *   routine allows replying to the sender of the current message 
 *
 *   Note that this routine generates automatic header information
 *   for the subject and (obviously) to lines, but that these can
 *   be altered while in the editor composing the reply message! 
 */


#include "headers.h"


#ifdef NLS
# define NL_SETN  34
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


int
reply()

{
	/*
	 *  Reply to the current message.  Returns non-zero iff
	 *  the screen has to be rewritten. 
	 */


	char            return_address[VERY_LONG_STRING],
			subject[SLEN];
	int             return_value;


#ifdef FORMS_MODE_SUPPORTED
	int             form_letter;

	form_letter = ( header_table[current - 1].status & FORM_LETTER );
#endif


	get_return( return_address, TRUE );

	if ( first_word(header_table[current - 1].from, "To:") ) {
		strcpy( subject, header_table[current - 1].subject );


#ifdef FORMS_MODE_SUPPORTED
		if ( form_letter )
			return_value = mail_filled_in_form( return_address, subject );
		else
#endif


			return_value = send_msg( return_address, "", 
						 subject, TRUE, NO, 1 );
	} else if ( header_table[current - 1].subject[0] != '\0' ) {
		if ( first_word(header_table[current - 1].subject, "Re:") ||
		     first_word(header_table[current - 1].subject, "RE:") ||
		     first_word(header_table[current - 1].subject, "re:")   )
			strcpy( subject, header_table[current - 1].subject );
		else {
			strcpy( subject, "Re: " );
			strcat( subject, header_table[current - 1].subject );
		}


#ifdef FORMS_MODE_SUPPORTED
		if ( form_letter )
			return_value = mail_filled_in_form( return_address, subject );
		else
#endif


			return_value = send_msg( return_address, "", 
						 subject, TRUE, NO, 1 );
	} else


#ifdef FORMS_MODE_SUPPORTED
	if ( form_letter )
		return_value = mail_filled_in_form( return_address,
						    "Filled in Form" );
	else
#endif


		return_value = send_msg( return_address, "", "Re: your mail",
				         TRUE, NO, 1 );

	clearit( header_table[current - 1].status, NEW );        /* it's been read now! */

	return ( return_value );
}


int
reply_to_everyone()

{
	/*
	 *  Reply to everyone who received the current message.  
	 *  This includes other people in the 'To:' line and people
	 *  in the 'Cc:' line too.  Returns non-zero iff the screen 
         *  has to be rewritten. 
	 */


	char            return_address[VERY_LONG_STRING], 
			subject[SLEN];
	char            full_address[VERY_LONG_STRING];
	int             return_value;


	get_return( return_address, TRUE );


#ifndef HAVE_GROUP_REPLIES_BE_A_CC
	full_address[0] = '\0';			/* no copies yet    */
#else
	strcpy( full_address, return_address );	/* sender gets copy */
#endif


	get_and_expand_everyone( return_address, full_address );

	if ( header_table[current - 1].subject[0] != '\0' ) {
		if ( first_word(header_table[current - 1].subject, "Re:") ||
		     first_word(header_table[current - 1].subject, "RE:") ||
		     first_word(header_table[current - 1].subject, "re:")   )
			strcpy( subject, header_table[current - 1].subject );
		else {
			strcpy( subject, "Re: " );
			strcat( subject, header_table[current - 1].subject );
		}

		return_value = send_msg( return_address, full_address, subject,
				         TRUE, NO, 2 );
	} else
		return_value = send_msg( return_address, full_address,
				         "Re: your mail", TRUE, NO, 2 );

	clearit( header_table[current - 1].status, NEW );        /* it's been read now! */

	return ( return_value );

}


int
forward()

{
	/*
	 *  Forward the current message. 
	 *  This calls 'send_msg()' to get the address and 
	 *  route the mail. 
	 */


	char            subject[SLEN], 
			address[VERY_LONG_STRING];

	int		results, 
			edit_msg = FALSE;


	forwarding = TRUE;

	address[0] = '\0';

	if ( header_table[current - 1].status & FORM_LETTER)
		PutLine0(LINES - 3, COLUMNS - 40, catgets(nl_fd,NL_SETN,1,"<no editing allowed>") );
	else {
		edit_msg = want_to(catgets(nl_fd,NL_SETN,2,"Edit outgoing message (y/n) ? "), 'y', TRUE);
		if ( edit_msg == 'y' )
			edit_msg = TRUE;
		else if ( edit_msg == 'n' )
			edit_msg = FALSE;
		else 
			return ( 0 );
	}

	if ( strlen(header_table[current - 1].subject) > 0 ) {

		strcpy( subject, header_table[current - 1].subject );

		/*
		 * this next strange compare is to see if the last few chars
		 * are already '(fwd)' before we tack another on 
		 */

		if ( strlen(subject) < 6 
		     || (strcmp((char *) subject + strlen(subject) - 5, "(fwd)") != 0) )
			strcat( subject, " (fwd)" );

		results = send_msg( address, "", subject, edit_msg,
			    	header_table[current - 1].status & FORM_LETTER ?
				PREFORMATTED : allow_forms, 0 );
	} else
		results = send_msg( address, "", "Forwarded mail", edit_msg,
			    	header_table[current - 1].status & FORM_LETTER ?
				PREFORMATTED : allow_forms, 0 );

	forwarding = FALSE;

	clearit( header_table[current - 1].status, NEW );        /* it's been read now! */

	return ( results );
}


int
get_and_expand_everyone( return_address, full_address )

	char            *return_address, 
			*full_address;

{
	/*
	 *  Read the current message, extracting addresses from the 'To:'
	 *  and 'Cc:' lines.   As each address is taken, ensure that it
	 *  isn't to the author of the message NOR to us.  If neither,
	 *  prepend with current return address and append to the 
	 *  'full_address' string.
	 */


	char            ret_address[VERY_LONG_STRING], 
			buf[VERY_LONG_STRING], 
			new_address[VERY_LONG_STRING], 
			address[VERY_LONG_STRING], 
			comment[LONG_SLEN];

	int             in_message = 1, 
			first_pass = 0, 
			indx, 
			line_pending = 0;


	/*
	 *  First off, get to the first line of the message desired 
	 */

	if ( fseek(mailfile, header_table[current - 1].offset, 0) == -1 ) {

		dprint( 1, (debugfile, "Error: seek %ld resulted in errno %s (%s)\n",
			header_table[current - 1].offset, error_name(errno),
			"get_and_expand_everyone") );
		error2( catgets(nl_fd,NL_SETN,3,"ELM [seek] couldn't read %d bytes into file (%s)"),
		        header_table[current - 1].offset, error_name(errno) );
		return;

	}

	/*
	 *  okay!  Now we're there!  
	 *
	 *  let's fix the ret_address to reflect the return address of this
    	 *  message with '%s' instead of the persons login name... 
	 */

	translate_return( return_address, ret_address );

	/*
	 *  now let's parse the actual message! 
	 */

	while ( in_message ) {
		if ( !line_pending )
			in_message = (int) ( fgets(buf, VERY_LONG_STRING, mailfile) != NULL );
		line_pending = 0;

		if ( real_from(buf, (struct header_rec *)0) && first_pass++ != 0 )
			in_message = FALSE;
		else if ( first_word(buf, "To:") || first_word(buf, "Cc:") ||
			 first_word(buf, "CC:") || first_word(buf, "cc:") ) {

			do {
				no_ret( buf );

				/*
				 *  we have a buffer with a list of addresses, each of 
				 *  either the form "address (name)" or "name <address>".
				 *  Our mission, should we decide not to be too lazy, 
				 *  is to break it into the two parts.
		      	  	 */

				if ( !whitespace(buf[0]) )

					/*
					 * skip header field 
					 */

					indx = chloc( buf, ':' ) + 1;	
				else
					indx = 0;	/* skip whitespace   */

				while ( break_down_tolist(buf, &indx, address, comment) ) {

					if ( okay_address(address, return_address) ) {
						if ( chloc( address, '@' ) == -1 )
							if ( strlen(new_address)
							     +strlen(ret_address) < VERY_LONG_STRING )
							     sprintf( new_address,ret_address,
								 address );
							else
							     new_address[0] = '\0';
						else
							strcpy( new_address, address );

						optimize_and_add(new_address,full_address);
					}

				}

				in_message = (int)(fgets(buf, VERY_LONG_STRING, mailfile) != NULL);

#ifdef DEBUG
				if ( in_message )
					dprint( 2, (debugfile, "> %s", buf) );
#endif

			} while ( in_message && whitespace(buf[0]) );

			line_pending++;

		} else if ( strlen(buf) < 2 )		/* done with header */
			in_message = FALSE;
	}
}


int
okay_address( address, return_address )

	char            *address, 
			*return_address;

{
	/*
	 *  This routine checks to ensure that the address we just got
	 *  from the "To:" or "Cc:" line isn't us AND isn't the person	
	 *  who sent the message.  Returns true iff neither is the case.
	 *  Special case added: if an alternative list entry contains a
	 *  leading '^' character (circumflex) then we'll assume it to 
	 *  mean the equivalent regular expression "[^!@a-zA-Z0-9]" at that
	 *  point.  For example:
 	 *
	 *	To: taylor, jin
 	 *
	 *  would find the first to *not* be an okay address if the user
	 *  had ^taylor in their alternatives list.
	 */


	char            our_address[VERY_LONG_STRING],
			comment[SLEN];
	struct addr_rec *alternatives;
	register int    loc;
	int		i = 0;


	if ( equal(address, return_address) )
		return ( FALSE );

	while ( break_down_tolist(return_address, &i, our_address, comment) ) {
		if ( chloc(our_address, '!') != -1 ||
                     chloc(our_address, '@') != -1    ) {
			if ( in_string(address, our_address) )
				return( FALSE );
		} else {	
			if ( equal(address, our_address) )
			return( FALSE );
		}
	}

	if ( strlen(hostname) + strlen(username) > VERY_LONG_STRING )
		return( TRUE );

	if ( equal(address, username) )
		return ( FALSE );

	sprintf( our_address, "%s!%s", hostname, username );

	if ( in_string(address, our_address) )
		return ( FALSE );

	sprintf( our_address, "%s@%s", username, hostname );

	if ( in_string(address, our_address) )
		return ( FALSE );

	alternatives = alternative_addresses;

	while ( alternatives != NULL ) {
		if ( alternatives->address[0] == CIRCUMFLEX ) {
			if ( (loc=in_string_indx(address, alternatives->address + 1)) > -1 )
				if ( loc == 0 || circumflex_match_char(address[loc - 1]) )
					return ( FALSE );

		} else {
			if ( chloc(alternatives->address, '!') != -1 ||
			     chloc(alternatives->address, '@') != -1   ) {
				if ( in_string(address, alternatives->address) )
					return ( FALSE );
			} else {
				if ( equal(address, alternatives->address) )
					return ( FALSE );
			}
		}
		alternatives = alternatives->next;
	}

	return ( TRUE );
}


int
optimize_and_add( new_address, full_address )

	char            *new_address, 
			*full_address;

{
	/*
	 *  This routine will add the new address to the list of addresses
	 *  in the full address buffer IFF it doesn't already occur.  It
	 *  will also try to fix dumb hops if possible, specifically hops
	 *  of the form ...a!b...!a... and hops of the form a@b@b etc 
	 */


	register int    len, 
			host_count = 0, 
			i;
	char            hosts[MAX_HOPS][SLEN];	/* array of machine names */
	char            *host, 
			*addrptr;


	if ( in_string(full_address, new_address) )
		return ( 1 );	/* duplicate address */

	/*
	 * optimize 
	 * break down into a list of machine names, checking as we go along 
	 */

	addrptr = (char *) new_address;

	while ( (host = get_token(addrptr, "!", 1, FALSE)) != NULL ) {
		for ( i = 0; i < host_count && !equal(hosts[i], host); i++ );

		if ( i == host_count ) {
			strcpy( hosts[host_count++], host );
			if ( host_count == MAX_HOPS ) {
				dprint( 2, (debugfile,
					"Error: hit max_hops limit trying to build return address (%s)\n",
					"optimize_and_add") );

				error( catgets(nl_fd,NL_SETN,4,"Can't build return address - hit MAX_HOPS limit!") );

				return ( 1 );
			}
		} else
			host_count = i + 1;

		addrptr = NULL;
	}

	/*
	 *  fix the ARPA addresses, if needed 
	 */

	if ( chloc(hosts[host_count - 1], '@') > -1 )
		fix_arpa_address( hosts[host_count - 1] );

	/*
	 *  rebuild the address.. 
	 */

	new_address[0] = '\0';

	for ( i = 0; i < host_count; i++ )
		sprintf( new_address, "%s%s%s", new_address,
			new_address[0] == '\0' ? "" : "!",
			hosts[i] );

	if ( full_address[0] == '\0' )
		strcpy( full_address, new_address );
	else {
		len = strlen( full_address );

		if ( len + strlen(new_address) + 2 < VERY_LONG_STRING ) {
			full_address[len] = ',';
			full_address[len + 1] = ' ';
			full_address[len + 2] = '\0';
			strcat( full_address, new_address );
		}
	}

	return ( 0 );
}


int
get_return_name( address, name, trans_to_lowercase )

	char            *address, 
			*name;
	int             trans_to_lowercase;

{
	/*
	 *  Given the address (either a single address or a combined list 
	 *  of addresses) extract the login name of the first person on
	 *  the list and return it as 'name'.  Modified to stop at
	 *  any non-alphanumeric character. 
         *
	 *  An important note to remember is that it isn't vital that this
	 *  always returns just the login name, but rather that it always
	 *  returns the SAME name.  If the persons' login happens to be,
	 *  for example, joe.richards, then it's arguable if the name 
	 *  should be joe, or the full login.  It's really immaterial, as
	 *  indicated before, so long as we ALWAYS return the same name!
	 *
	 *  Another note: modified to return the argument as all lowercase
	 *  always, unless trans_to_lowercase is FALSE... 
	 */


	char            single_address[VERY_LONG_STRING];
	register int    i, 
			loc, 
			indx = 0;


	dprint( 6, (debugfile, "get_return_name called with (%s, <>, shift=%s)\n",
		   address, onoff(trans_to_lowercase)) );

	/*
	 * First step - copy address up to a comma, space, or EOLN 
	 */

	for ( i = 0; address[i] != ',' && !whitespace(address[i]) &&
	     	address[i] != '\0' && i < VERY_LONG_STRING ; i++ )
		single_address[i] = address[i];

	single_address[i] = '\0';

	/*
	 * Now is it an ARPA address?? 
	 */

	if ( (loc = chloc(single_address, '@')) != -1 ) {	/* Yes */

		/*
		 * At this point the algorithm is to keep shifting our copy
		 * window left until we hit a '!'.  The login name is then
		 * located between the '!' and the first metacharacter to
		 * it's right (ie '%', ':' or '@'). 
		 */

		for ( i = loc; single_address[i] != '!' && i > -1; i-- )
			if ( single_address[i] == '%' ||
			     single_address[i] == ':' ||
			     single_address[i] == '.' ||	/* no domains */
			     single_address[i] == '@' )
				loc = i - 1;

		if ( i < 0 || single_address[i] == '!' )
			i++;

		for ( indx = 0; indx < loc - i + 1; indx++ )
			if ( trans_to_lowercase )
				name[indx] 
				= (char)tolower( (int)single_address[indx + i] );
			else
				name[indx] = single_address[indx + i];

		name[indx] = '\0';

		/* Kludge for X.400 addresses: if it starts with a '/', skip
		 * that.  If there is an '=', advance past that if the next
		 * character is alpha-numeric.
		 */
		if ((strlen(name) >= 1) && (name[0] == '/')){
			char *temp_name;

			if((temp_name = (char *)malloc(strlen(name) + 1)) != NULL) {
				strcpy(temp_name, name+1);
				i = chloc(temp_name, '=');
				if ((i != -1) && (strlen(temp_name) > i+1) && isalnum(temp_name[i+1]))
					temp_name += i+1;
				strcpy(name, temp_name);
				free (temp_name);
			} else {
				dprint (1, (debugfile,
						"ERROR: Couldn't malloc space to parse X.400 address\n"));
			}
		}

	} else {				/* easier - standard USENET address */

		/*
		 * This really is easier - we just cruise left from the end
		 * of the string until we hit either a '!' or the beginning
		 * of the line.  No sweat. 
		 */

		loc = strlen( single_address ) - 1;		/* last char */

		for ( i = loc; single_address[i] != '!' && single_address[i] != '.'
		     					&& i > -1; i-- )

			if ( trans_to_lowercase )
				name[indx++] 
				=(char)tolower((int)single_address[i]);
			else
				name[indx++] = single_address[i];

		name[indx] = '\0';
		reverse( name );
	}

        /* Take everything up to, but not including, the next non-alpha-numeric.
         */
	for (i=0; name[i] && isalnum(name[i]); i++)
        	;
	name[i] = '\0';

	dprint( 6, (debugfile, "get_return_name() returning '%s'.\n", name));
}


int
break_down_tolist( buf, indx, address, comment )

	char            *buf, 
			*address, 
			*comment;
	int             *indx;

{
	/*
	 *  This routine steps through "buf" and extracts a single address
	 *  entry.  This entry can be of any of the following forms;
  	 *
	 *	address (name)
	 *	name <address>
	 *	address
	 *
	 *  Once it's extracted a single entry, it will then return it as
	 *  two tokens, with 'name' (e.g. comment) surrounded by parens.
	 *  Returns ZERO if done with the string...
	 */


	char            buffer[VERY_LONG_STRING];
	register int    i, 
			loc = 0, 
			angle_brackets = 0,
			parentheses = 0,
			quote_mode = 0,
			hold_index;


	if ( *indx > strlen(buf) )
		return ( FALSE );

	while ( whitespace(buf[*indx]) )
		( *indx )++;

	if ( *indx > strlen(buf) )
		return ( FALSE );

	/*
	 *  Now we're pointing at the first character of the token! 
	 */

	hold_index = *indx;

	while ( buf[*indx] != '\0' ) {
		if ( buf[*indx] == '"' && !quote_mode)
			quote_mode++;
		/* We don't need to verify that (*indx)>0 here, since it has
		 * to be in order for us to be in quote_mode.
		 */
		else if ( quote_mode && buf[*indx] == '"' && buf[(*indx)-1] != '\\')
			quote_mode = 0;
		if ( !quote_mode && buf[*indx] == '<')
			angle_brackets++;
		if ( !quote_mode && buf[*indx] == '>')
			angle_brackets--;
		if ( !quote_mode && buf[*indx] == '(')
			parentheses++;
		if ( !quote_mode && buf[*indx] == ')')
			parentheses--;
		if ( !quote_mode && angle_brackets == 0 && parentheses == 0 && buf[*indx] == ',' )
			break;
		buffer[loc++] = buf[(*indx)++];
	}
	( *indx )++;
	buffer[loc] = '\0';

	while ( whitespace(buffer[loc-1]) )
		buffer[--loc] = '\0'; /* remove trailing while space */

	if ( !buffer[0] )
		return ( FALSE );

	dprint( 10, (debugfile, "\n* got \"%s\"\n", buffer) );

	if ( buffer[loc - 1] == ')' ) {		/* address (name)  format */
		for ( loc = 0; buffer[loc] && buffer[loc] != '('; loc++ );

			/*
			 * get to the opening comment character... 
			 */

		loc--;				/* back up to just before the paren */

		while ( whitespace(buffer[loc]) )
			loc--;			/* back up */

		/*
		 *  get the address field...
		 */

		for ( i = 0; i <= loc; i++ )
			address[i] = buffer[i];

		address[i] = '\0';

		/*
		 *  now get the comment field! 
		 */

		loc = 0;

		for ( i = chloc(buffer, '('); buffer[i]; i++ )
			comment[loc++] = buffer[i];

		comment[loc] = '\0';

	} else if ( buffer[loc - 1] == '>' ) {	 	/* name <address>  format */

		dprint( 7, (debugfile, "\tcomment <address>\n") );

		for ( loc = 0; buffer[loc] != '<' && buffer[loc]; loc++ );

			/*
		 	 * get to the opening comment character... 
			 */

		while ( whitespace(buffer[loc]) )
			loc--;				/* back up */

		/*
		 *  get the comment field... 
		 */

		comment[0] = '(';

		for ( i = 1; i < loc; i++ )
			comment[i] = buffer[i - 1];

		comment[i++] = ')';
		comment[i] = '\0';

		/*
		 *  now get the address field !
		 */

		loc = 0;

		for ( i = chloc(buffer, '<') + 1; buffer[i] && buffer[i+1]; i++ )
			address[loc++] = buffer[i];

		address[loc] = '\0';

	} else {
		/*
		 *  the next section is added so that all To: lines have commas
      	         *  in them accordingly 
		 */

		for ( i = 0; buffer[i] != '\0'; i++ )
			if ( whitespace(buffer[i]) )
				break;

		if ( buffer[i] ) {		/* shouldn't be whitespace */
			buffer[i] = '\0';
			*indx = hold_index + i + 1;
		}

		strcpy( address, buffer );
		comment[0] = '\0';
	}

	dprint( 10, (debugfile, "-- returning '%s' '%s'\n", address, comment) );

	return ( TRUE );
}
