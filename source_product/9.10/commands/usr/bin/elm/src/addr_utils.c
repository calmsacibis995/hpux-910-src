/**			addr_utils.c			**/

/*
 *  @(#) $Revision: 70.4 $	
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  This file contains addressing utilities 
 */

#include <sys/types.h>
#include <sys/stat.h>

#include "headers.h"


#ifdef NLS
# define NL_SETN  1
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


int
talk_to( sitename )

	char           *sitename;

{
	/*
	 *  If we talk to the specified site, return true, else
	 *  we're going to have to expand this sitename out, so 
	 *  return false! 
	 */


	struct lsys_rec *sysname;
	

	sysname = talk_to_sys;

	if ( sysname == NULL ) {
		dprint(2, (debugfile,
		       "Warning: talk_to_sys is currently set to NULL!\n"));
		return ( 0 );
	}

	while ( sysname != NULL ) {
		if ( equal( sysname->name, sitename ) )
			return ( 1 );
		else
			sysname = sysname->next;
	}

	return ( 0 );
}


int 
add_site( buffer, site, lastsite )

	char           	*buffer, 
			*site, 
			*lastsite;

{
	/*
	 *  add site to buffer, unless site is 'uucp', current 
	 *  machine, or site is the same as lastsite.   If not, 
	 *  set lastsite to site.
	 */


	char            local_buffer[VERY_LONG_STRING];
	char           	*strip_parens();


	if ( !equal( site, "uucp" ) )
		if ( !equal( site, lastsite ) )
			if ( !equal( site, hostname ) ) {

				if ( buffer[0] == '\0' )

					/* first in list ! */

					strcpy( buffer, strip_parens( site ) );
				else {
					sprintf( local_buffer, "%s!%s", buffer, 
								strip_parens( site ) );
					if ( strlen(local_buffer) < VERY_LONG_STRING )
					   strcpy( buffer, local_buffer );
				}

				/* don't want THIS twice ! */

				strcpy( lastsite, strip_parens( site ) );	
								
			}

}


#ifdef USE_EMBEDDED_ADDRESSES

int 
get_address_from( prefix, line, buffer )

	char           	*prefix, 
			*line, 
			*buffer;

{
	/*
	 *  This routine extracts the address from either a 'From:' line
	 *  or a 'Reply-To:' line...the algorithm is quite simple, too:
	 *  increment 'line' past header, then check last character of 
	 *  line.  If it's a '>' then the address is contained within '<>'
	 *  and if it's a ')' then the address is in the 'clear'... 
	 */

	register char	address1[VERY_LONG_STRING],
			comment[LONG_SLEN];
	int		i = 0; 


	no_ret( line );

	line = (char *) (line + strlen( prefix ) + 1);

	buffer[0] = '\0';
	if ( in_string( line, "<" ) ) {
		while ( break_down_tolist( line, &i, address1, comment ) ) {
			strcat( buffer, address1 );	
			if ( equal(prefix, "Reply-To:") ) {
				strcat( buffer, " " );
				strcat( buffer, comment );	
			}
			strcat( buffer, ", " );
		}
		buffer[strlen(buffer) - 2] = '\0';

	} else 			/* either ')' or address in the clear... */
		strcpy( buffer, line );
}

#endif


int 
translate_return( addr, ret_addr )

	char          	*addr, 
			*ret_addr;

{
	/*
	 *  Return ret_addr to be the same as addr, but with the login 
         *  of the person sending the message replaced by '%s' for 
         *  future processing... 
	 *  Fixed to make "%xx" "%%xx" (dumb 'C' system!) 
	 */


	register int    loc, 
			loc2, 
			indx = 0;


	loc2 = chloc( addr, '@' );

	if ( (loc = chloc( addr, '%' )) < loc2 )
		loc2 = loc;

	if ( loc2 != -1 ) {		
	
		/*
		 * ARPA address:
		 * algorithm is to get to '@' sign and move backwards until
		 * we've hit the beginning of the word or another metachar. 
		 */

		for ( loc = loc2 - 1; loc > -1 && addr[loc] != '!'; loc-- );

	} else {		

		/*
		 * usenet address:
		 * simple algorithm - find last '!' 
		 */

		loc2 = strlen( addr );		/* need it anyway! */

		for ( loc = loc2; loc > -1 && addr[loc] != '!'; loc-- );
	}

	/*
	 *  now copy up to 'loc' into destination... 
	 */

	while ( indx <= loc ) {
		ret_addr[indx] = addr[indx];
		indx++;
	}

	/*
	 *  now append the '%s'... 
	 */

	ret_addr[indx++] = '%';
	ret_addr[indx++] = 's';

	/*
	 *  and, finally, if anything left, add that 
	 */

	while ( loc2 < strlen( addr ) ) {

		ret_addr[indx++] = addr[loc2++];

		if ( addr[loc2 - 1] == '%' )	/* tweak for "printf" */
			ret_addr[indx++] = '%';
	}

	ret_addr[indx] = '\0';
}


int
build_address( to, full_to )

char 		*to, 
		*full_to;

{
	/*
	 *  loop on all words in 'to' line...append to full_to as
	 *  we go along, until done or length > len.  Modified to
	 *  know that stuff in parens are comments...Returns non-zero
	 *  if it changed the information as it copied it across...
	 */


	int		i = 0;
	register int 	comments = FALSE,
			changed = 0, 
#ifdef DEBUG
			in_parens = 0, 
#endif
			expanded_information = 0;
	char 		word[VERY_LONG_STRING], 
			*ptr;


#ifdef  ENABLE_NAMESERVER
	char 		rebuilt_to_list[VERY_LONG_STRING];
#endif


#ifndef NOCHECK_VALIDNAME
	char 		buffer[VERY_LONG_STRING];	/* used if we're going to check the name */
#endif


	char 		new_to_list[VERY_LONG_STRING];


	dprint( 6, (debugfile, "\nin build-address: to='%s'\n", to) );
	dprint( 6, (debugfile, " ... and full-to='%s'\n", full_to) );

	new_to_list[0] = '\0';


#ifdef ENABLE_NAMESERVER
	rebuilt_to_list[0] = '\0';
#endif


	if ( parse_rfc822( to ) < 0 ) 		/* make it canonical         */
		return( 0 );

	strip_dup_addr( to );			/* remove duplicated address */

	comments = get_word( to, &i, word );

	full_to[0] = '\0';


	/*
	 *  The following while loop is to try to do something intelligent
	 *  with each word of the address.  The algorithm here is, though
	 *  it may not seem to be:
	 *
	 *  For each word we extract from the given string
	 *
	 *	  1. If it starts a comment, or is a comment itself
	 *           (eg. a parenthesized expression), copy it across.
	 *
 	 *	  2. If it's a complex address (that is, one that has
	 *	     either a UUCP or SMTP route specified ('!' or '@')
	 *	     or a DEC/X.400 value (':')) then add it.
	 *
	 *	  3. If it's a '?' and we're allowing X.400, prompt
	 *	     the user for the address.
	 *
	 *	  4. If it's an alias, expand it and add it to the
	 *	      expanding output string.
	 *
	 *	  5. Else if it exists, either hand it to the user
	 *	     defined name server or check to see if it can
	 *	     be delivered locally or copy it across blindly,
	 *	     or reject it.
	 */
	

	while ( i <= (int)strlen( to ) ) {

		dprint( 2, (debugfile,"<got word '%s' with in-parens %s>\n",
			  word, in_parens? "ON" : "OFF") );

		if ( comments ) {
			sprintf( full_to, "%s%s%s", full_to,
				 strlen( full_to ) ? " ": "", word );


#ifdef ENABLE_NAMESERVER
			if ( rebuilt_to_list[0] != '\0' )
				strcat( rebuilt_to_list, " " );

			strcat( rebuilt_to_list, word );
			dprint( 2, (debugfile, "build-addr adding: '%s'\n", word) );
#endif


		} else if ( strpbrk( word,"!@:" ) != NULL ) {


#ifdef DONT_TOUCH_ADDRESSES
			sprintf( full_to, "%s%s%s", full_to,
				 full_to[0] != '\0'? "," : "", word );
#else
			sprintf( full_to, "%s%s%s", full_to,
				full_to[0] != '\0'? "," : "", expand_system(word, 1) );
#endif


#ifdef ENABLE_NAMESERVER
			dprint( 3, (debugfile, "** adding '%s' to the string:\n> %s\n**\n",
				   word, rebuilt_to_list) );

			if ( rebuilt_to_list[0] != '\0' )
				strcat( rebuilt_to_list, "," );

			strcat( rebuilt_to_list, word );
#endif


		} else if ( (ptr = get_alias_address( word, 1, 0 )) != NULL ) {
			if ( strlen( full_to ) + strlen( ptr ) > VERY_LONG_STRING )
				break;

			sprintf( full_to, "%s%s%s", full_to, 
				 full_to[0] != '\0'? "," : "", ptr );

			expanded_information++;


#ifdef ENABLE_NAMESERVER
			if ( rebuilt_to_list[0] != '\0' )
				strcat( rebuilt_to_list, "," );

			strcat( rebuilt_to_list, word );

			dprint( 3, (debugfile, "** 1: appended '%s' to '%s'\n",
		 	           word, rebuilt_to_list) );
#endif


		} else if ( strlen(word) > 0 ) {


#ifdef ENABLE_NAMESERVER
			if ( check_nameserver( word ) ) {
				sprintf( full_to, "%s%s%s", full_to,
					full_to[0] != '\0'? "," : "", word );

				if ( rebuilt_to_list[0] != '\0' )
					strcat( rebuilt_to_list, "," );

				strcat( rebuilt_to_list, word );

				dprint( 3, (debugfile, "** 2: appended '%s' to '%s'\n",
					   word, rebuilt_to_list) );

				expanded_information++;
			} else {
#endif


#ifdef NOCHECK_VALIDNAME
			sprintf( full_to, "%s%s%s", full_to,
				 full_to[0] != '\0'? "," : "", word );
#ifdef ENABLE_NAMESERVER


				if ( rebuilt_to_list[0] != '\0' )
					strcat( rebuilt_to_list, "," );

				strcat( rebuilt_to_list, word );

				dprint( 3, (debugfile, "** 3: appended '%s' to '%s'\n",
					   word, rebuilt_to_list) );
			}
#endif

#else
			if ( valid_name(word) ) 
				sprintf( full_to, "%s%s%s", full_to,
					full_to[0] != '\0'? "," : "", word );

			else if ( check_only ) {
				printf( catgets(nl_fd,NL_SETN,1,"(alias \"%s\" is unknown)\n\r"), word );
				changed++;

			} else if ( ! isatty(fileno(stdin)) ) {	/* batch mode error! */
				fprintf(stderr,
					catgets(nl_fd,NL_SETN,2,"Cannot expand alias '%s'!\n\r"), word);
				fprintf(stderr,
				        catgets(nl_fd,NL_SETN,3,"Use \"elmalias -c\" to find valid addresses!\n\r"));
				dprint( 1, (debugfile,
					"Can't expand alias %s - bailing out of build_address\n", 
					word) );
				emergency_exit();

			} else {
				dprint( 2,(debugfile,"Entered unknown address %s\n", word) );
				sprintf( buffer, 
					catgets(nl_fd,NL_SETN,4,"'%s' is an unknown address.  Replace with: "), 
					word );
				word[0] = '\0';

				if ( mail_only ) 
					printf( buffer );
				else
					PutLine0( LINES, 0, buffer );
		
				(void) optionally_enter(word, LINES, strlen(buffer), FALSE);

				if ( strlen( word ) > 0 ) {


#ifdef ENABLE_NAMESERVER
					if ( rebuilt_to_list[0] != '\0' )
						strcat( rebuilt_to_list, "," );

					strcat( rebuilt_to_list, word );
#else
					sprintf( new_to_list, "%s%s%s", new_to_list,
						strlen( new_to_list ) > 0? ",":"", word );
					dprint( 3,(debugfile, "Replaced with %s in build_address\n", 
						word));
#endif


				} else
					dprint(3,(debugfile, 
					"Address removed from TO list by build_address\n"));

				if ( mail_only )
					printf( "\n\r" );

				changed++;
				clear_error();
				continue;
			}
#endif


		}

		if ( i == -1 )
			break;
		else
			comments = get_word( to, &i, word );
	}


#ifdef ENABLE_NAMESERVER
	strcpy( to, rebuilt_to_list );
#else
	if ( changed )
	  strcpy( to, new_to_list );
#endif


	dprint( 2,(debugfile, "leaving build-address:\n to='%s'\n", to) );
	dprint( 2,(debugfile, " full-to='%s'\n expanded-information = %d\n\n", 
		  full_to, expanded_information) );

	return( expanded_information > 0 ? 1 : 0 );
}


int
parse_rfc822( addr_buffer )

	char		*addr_buffer;

{
	/*
	 * This is parser make address-list canonical to distinguish comment
	 * from address.
	 * In this routine, following set of RFC822 is recognized.
	 *    1). string in parentheses are recognized as comment.
	 *    2). continuous delimiters are replaced to one delimiter.
	 *    3). sensitive delimiters are ' ', ',', '\t' and '\0'.
	 *    4). Comment nest, so that if an unquoted left parenthesis occurs
	 *        in a comment string, there must also be a matching right parenthesis.
	 *    5). Comment acts as the delimiter and equivalent with a single SPACE.
	 *        (Where SPACE is ' ' or ',' or '\t' or '\0')
	 *
	 * Through this parser, address and address or address and comment are
	 * separeated by one SPACE character.
	 *
	 * 	address1 ,, \t ,address2(comment1    ,,, comment2 )address3
	 *
	 * will changed to:
	 *
	 * 	address1 address2 (comment1    ,,, comment2 ) address3
	 *
	 * part of comment is not changed at all.
 	 */


	int		indx = 0,		/* indx in the addr_list */
			in_parens = 0;		/* in parentheses ?       */
	char		delimiter,		/* detected delimiter     */
			buffer[VERY_LONG_STRING],		/* work area              */
			word[VERY_LONG_STRING];		/* word taken from list   */


	if ( strlen( addr_buffer ) == 0 )	/* ensure address not empty */
		return( -2 );

	buffer[0] = '\0';

	while ( indx < strlen( addr_buffer ) + 1 ){

		/*
		 *   search delimiter in the address list
		 */

		indx = search_delimiter( addr_buffer, indx, word );


		if ( indx >= 0 )
			delimiter = addr_buffer[ indx ];
		else
			delimiter = '\0';


		if ( delimiter == '(' ){

			if ( indx == 0 ) {
				buffer[0] = '(';
				buffer[1] = '\0';
			} else if ( in_parens )
				sprintf( buffer, "%s%s%s%s%c", buffer, word, "(", '\0' );
			else
				sprintf( buffer, "%s%s%s%s%c", buffer,
					 strlen(buffer) && strlen(word)>0 ? " ":"", 
					 word, " (", '\0' );
			in_parens++;


		} else if ( delimiter == ')' ){

			if ( --in_parens < 0 )
				return( -1 );
			else
				sprintf( buffer, "%s%s%s%c", buffer, word, ")", '\0' );


		} else if ( in_parens )

			sprintf( buffer, "%s%s%c%c", buffer, word, 
				 delimiter, '\0' );


		else if ( strlen( word ) != 0 )
			sprintf( buffer, "%s%s%s%c", buffer,
				 strlen(buffer)>0 ? " ":"", word, '\0' );


		if ( indx == -1 ){ 		/* Now we get to the end  */

		/*
		 *      If we check the number of parenthesis, 
		 *      (enclosed in matching parentheses ?)
		 *	then, append this part.
		 *
		 *	if ( in_parens )
		 *		return( -1 );
		 *	else {
		 */

				strcpy( addr_buffer, buffer );
				return( 0 );
		/*
		 *	}
		 */

		}

		indx++;

	}

	return( -1 );    			/* Overshoot ???          */
}


int
real_from( buffer, entry )

	char   		        *buffer;
	struct header_rec 	*entry;

{
	/*
	 *     Returns true iff 's' has the seven 'from' fields, (or
	 *     8 - some machines include the TIME ZONE!!!)
	 *     Initializing the date and from entries in the record 
	 *     and also the message received date/time.  
	 */


	struct header_rec       temp,
	                        *rec;
	char            junk[STRING], 
			timebuff[STRING], 
			holding_from[VERY_LONG_STRING];
	int             eight_fields = 0;


	if ( !first_word(buffer, "From ") )
		return ( FALSE );


	if ( entry == '\0' )
		rec = &temp;
	else
		rec = entry;

	rec->year[0] = '\0';
	junk[0] = timebuff[0] = '\0';

	/*
	 * From <user> <day> <month> <date> <hr:min:sec> [<TZ name>] <year> 
	 */

	sscanf( buffer, "%*s %*s %*s %*s %*s %s %*s %s", timebuff, junk );

	if ( strlen(timebuff) < 3 ) {
		dprint( 3, (debugfile,
		       "Real_from returns FAIL [bad time field] on\n-> %s\n", buffer) );
		return ( FALSE );
	}

	if ( timebuff[1] != ':' && timebuff[2] != ':' ){
		dprint( 3, (debugfile,
		       "Real_from returns FAIL [bad time field] on\n-> %s\n", buffer) );
		return ( FALSE );
	}

	if ( junk[0] != '\0' ) {	/* try for 8 field entry */
		junk[0] = '\0';
		sscanf( buffer, "%*s %*s %*s %*s %*s %s %*s %*s %s", timebuff, junk );

		if ( junk[0] != '\0' ) {
			dprint( 3, (debugfile,
			    "Real_from returns FAIL [too many fields] on\n-> %s\n", buffer));
			return ( FALSE );
		} else 
			eight_fields++;
	}

	/*
	 *  now get the info out of the record! 
	 */

	if ( eight_fields ) {

		sscanf( buffer, "%s %s %s %s %s %s %*s %s", 
			junk, holding_from, rec->dayname, rec->month,
			rec->day, rec->time, rec->year );

		if ( isdigit(rec->month[0]) )
			sscanf( buffer, "%s %s %s %s %s %s %s %*s",
		       		junk, holding_from, rec->dayname, rec->day,
				rec->month, rec->year, rec->time );
	} else {
		sscanf( buffer, "%s %s %s %s %s %s %s",
			junk, holding_from, rec->dayname, rec->month,
		       	rec->day, rec->time, rec->year );

		if ( isdigit(rec->month[0]) )
			sscanf( buffer, "%s %s %s %s %s %s %s",
				junk, holding_from, rec->dayname, rec->day,
		       		rec->month, rec->year, rec->time );
	}

	strncpy( rec->from, holding_from, VERY_LONG_STRING );
	rec->from[VERY_LONG_STRING-1] = '\0';
	resolve_received( rec );
	return ( rec->year[0] != '\0' );

}


int
forwarded( buffer, entry )

	char        		*buffer;
	struct header_rec	*entry;

{
	/*
	 *  Change 'from' and date fields to reflect the ORIGINATOR of 
	 *  the message by iteratively parsing the >From fields... 
	 *  Modified to deal with headers that include the time zone
	 *  of the originating machine... 
	 */


	char            machine[SLEN], 
			buff[VERY_LONG_STRING], 
			holding_from[VERY_LONG_STRING];


	machine[0] = holding_from[0] = '\0';

	sscanf( buffer, "%*s %s %s %s %s %s %s %*s %*s %s",
	        holding_from, entry->dayname, entry->month,
	        entry->day, entry->time, entry->year, machine );

	if ( isdigit(entry->month[0]) )  {
		if ( equal(machine,"by") || equal(machine,"from") )
			sscanf( buffer, 
				"%*s %s %s %s %s %s %s %*s %*s %s",
		        	holding_from, entry->dayname,entry->day,
				entry->month, entry->year, entry->time,
				machine );
		else		 /* try for address including tz */
			sscanf( buffer, 
				"%*s %s %s %s %s %s %s %*s %*s %*s %s",
		        	holding_from, entry->dayname,entry->day,
		        	entry->month, entry->year, entry->time,
				machine );
	} else {
		if ( isdigit(entry->year[0]) )
			sscanf( buffer, 
				"%*s %s %s %s %s %s %s %*s %*s %s",
		        	holding_from, entry->dayname, entry->month,
				entry->day, entry->time, entry->year,
				machine );
		else
			sscanf( buffer, 
				"%*s %s %s %s %s %s %*s %s %*s %*s %s",
		        	holding_from, entry->dayname, entry->month,
		        	entry->day, entry->time, entry->year,
				machine );
	}
	

	/*
	 * the following fix is to deal with ">From xyz ... forwarded by xyz"
	 */

	if ( equal(machine, holding_from) )
		machine[0] = '\0';

	if ( machine[0] == '\0' )
		strcpy( buff, holding_from[0] ? holding_from : "anonymous" );
	else
		sprintf( buff, "%s!%s", machine, holding_from );

	strncpy( entry->from, buff, VERY_LONG_STRING );
	entry->from[VERY_LONG_STRING-1] = '\0';
	resolve_received( entry );

}


int 
parse_arpa_from( buffer, newfrom )

	char            *buffer, 
			*newfrom;

{
	/* 
	 *  try to parse the 'From:' line given... It can be in one of
	 *  two formats:
         *  From: Dave Taylor <hplabs!dat>
	 *  or  From: hplabs!dat (Dave Taylor)
	 */


	char            temp_buffer[VERY_LONG_STRING], 
			*temp;
	register int    i, 
			j = 0;


	temp = (char *) temp_buffer;
	temp[0] = '\0';

	no_ret( buffer );		/* blow away '\n' char! */

	if ( lastch(buffer) == '>' ) {
		for ( i = strlen("From: ");
		      buffer[i] != '\0' && buffer[i] != '<' && buffer[i] != '('; 
		      i++ )
			temp[j++] = buffer[i];
		temp[j] = '\0';

	} else if (lastch(buffer) == ')') {
		for (i = strlen(buffer) - 2; buffer[i] != '\0' && buffer[i] != '(' &&
		     buffer[i] != '<'; i--)
			temp[j++] = buffer[i];
		temp[j] = '\0';
		reverse( temp );
	}
	

#ifdef USE_EMBEDDED_ADDRESSES

	/*
	 *  if we have a null string at this point, we must just have a 
	 *  From: line that contains an address only.  At this point we
	 *  can have one of a few possibilities...
         *
 	 *	From: address
	 *	From: <address>
	 *	From: address ()
	 */

	if ( strlen(temp) == 0 ) {
		if ( lastch(buffer) != '>' ) {
			for ( i = strlen("From:"); 
			      buffer[i] != '\0' && buffer[i] != '('; i++ )
				temp[j++] = buffer[i];
			temp[j] = '\0';

		} else {		/* get outta '<>' pair, please! */
			for ( i = strlen(buffer) - 2; 
			      buffer[i] != '<' && buffer[i] != ':'; i-- )
				temp[j++] = buffer[i];
			temp[j] = '\0';
			reverse( temp );
		}
	}
#endif


	if ( strlen(temp) > 0 ) {	

		/* 
		 * mess with buffer...
		 * remove leading spaces and quotes... 
		 */

		while ( whitespace(temp[0]) || quote(temp[0]) )
			temp = (char *) (temp + 1);	/* increment address! */

		/*
		 * remove trailing spaces and quotes... 
		 */

		i = strlen( temp ) - 1;

		while ( whitespace(temp[i]) || quote(temp[i]) )
			temp[i--] = '\0';

		/*
		 * if anything is left, let's change 'from' value! 
		 */

		if ( strlen(temp) > 0 )
			strncpy( newfrom, temp, VERY_LONG_STRING );
	}
}


int 
parse_arpa_date( string, entry )

	char          		 *string;
	struct header_rec	 *entry;

{
	/*
	 *  Parse and figure out the given date format... return
	 *  the entry fields changed iff it turns out we have a
	 *  valid parse of the date!  
	 */


	char            word[15][NLEN], 
			buffer[VERY_LONG_STRING], 
			*bufptr;
	char            *aword;
	int             words = 0;


	strcpy(buffer, string);
	bufptr = (char *) buffer;

	/*
	 *  break the line down into words... 
	 */

	while ( (aword = strtok(bufptr, " \t '\"-/(),.")) != NULL ) {
		strcpy( word[words++], aword );
		bufptr = NULL;
	}

	if ( words < 6 ) {		/* strange format.  We're outta here! */
		dprint( 3, (debugfile,
		        "Parse_arpa_date failed [less than six fields] on\n-> %s\n",
			string) );
		return;
	}

	/*
	 * There are now five possible combinations that we could have: 
	 *
	 * Date: day_number month_name year_number time timezone Date: day_name
	 * day_number month_name year_number ... Date: day_name month_name
	 * day_number time year_number Date: day_name month_name day_number
	 * year_number time Date: day_number month_name year_number time
	 * timezone day_name 
	 *
	 * Note that they are distinguishable by checking the first character of
	 * the second, third and fourth words... 
	 */

	if ( isdigit(word[1][0]) ) {		/*** type one! ***/
		if ( !valid_date(word[1], word[3]) ) {
			dprint(3, (debugfile,
				   "parse_arpa_date failed [bad date: %s/%s/%s] on\n-> %s\n",
				   word[1], word[2], word[3], string));
			return;			/* strange date! */
		}

		strncpy( entry->day, word[1], 3 );
		strncpy( entry->month, word[2], 3 );
		strncpy( entry->year, word[3], 4 );
		strncpy( entry->time, word[4], 10 );

	} else if ( isdigit(word[2][0]) ) {	/*** type two! ***/
		if ( !valid_date(word[2], word[4]) ) {
			dprint( 3, (debugfile,
				   "parse_arpa_date failed [bad date: %s/%s/%s] on\n-> %s\n",
				   word[2], word[3], word[4], string) );
			return;			/* strange date! */
		}

		strncpy( entry->day, word[2], 3 );
		strncpy( entry->month, word[3], 3 );
		strncpy( entry->year, word[4], 4 );
		strncpy( entry->time, word[5], 10 );

	} else if ( isdigit(word[3][0]) ) {
		if ( word[4][1] == ':' || word[4][2] == ':' ) {	/*** type three! ***/
			if (!valid_date(word[3], /* word[2], */ word[5])) {
				dprint( 3, (debugfile,
					   "parse_arpa_date failed [bad date: %s/%s/%s] on\n-> %s\n",
					   word[3], word[2], word[5], string) );
				return;		/* strange date! */
			}

			strncpy( entry->year, word[5], 4 );
			strncpy( entry->time, word[4], 10 );

		} else {			/*** type four!  ***/
			if ( !valid_date(word[3], word[4]) ) {
				dprint( 3, (debugfile,
					   "parse_arpa_date failed [bad date: %s/%s/%s] on\n-> %s\n",
					   word[3], word[2], word[4], string) );
				return;		/* strange date! */
			}

			strncpy( entry->year, word[4], 4 );
			strncpy( entry->time, word[5], 10 );
		}

		strncpy( entry->day, word[3], 3 );
		strncpy( entry->month, word[2], 3 );
	}

	/*
	 *  finally, let's just normalize the monthname to be a three
	 *  letter abbreviation, with the first capitalized and the
	 *  second and third in lowercase... 
	 */

	shift_lower( entry->month );
	entry->month[0] = toupper( entry->month[0] );
}


int 
fix_arpa_address( address )

	char           *address;

{
	/*
	 *  Given a pure ARPA address, try to make it reasonable.
         *
	 *  This means that if you have something of the form a@b@b make 
         *  it a@b.  If you have something like a%b%c%b@x make it a%b@x...
	 */

	register int    host_count = 0, 
			i;
	char            hosts[MAX_HOPS][200 * NLEN];	/* array of machine
							 * names */
	char            *host, 
			*addrptr;


	/*
	 * break down into a list of machine names, checking as we go along 
	 */

	addrptr = (char *) address;

	while ( (host = get_token(addrptr, "%@", 2, FALSE)) != NULL ) {
		for ( i = 0; i < host_count && !equal(hosts[i], host); i++ );

		if ( i == host_count ) {
			strcpy( hosts[host_count++], host );

			if ( host_count == MAX_HOPS ) {
				dprint( 2, (debugfile,
					   "Can't build return address - hit MAX_HOPS in fix_arpa_address\n") );
				error( catgets(nl_fd,NL_SETN,5,"Can't build return address - hit MAX_HOPS limit!") );
				return ( 1 );
			}
		} else
			host_count = i + 1;

		addrptr = NULL;
	}

	/*
	 *  rebuild the address.. 
	 */

	address[0] = '\0';

	for ( i = 0; i < host_count; i++ )
		sprintf( address, "%s%s%s", address,
			 address[0] == '\0' ? "" :
			 (i == host_count - 1 ? "@" : "%"),
			 hosts[i] );

	return ( 0 );
}


int 
figure_out_addressee( buffer, mail_to )

	char           *buffer,
	               *mail_to;

{
	/*
	 *  This routine steps through all the addresses in the "To:"
	 *  list, initially setting it to the first entry (if mail_to
	 *  is NULL) or, if the user is found (eg "alternatives") to
	 *  the current "username".
         *
	 *  Modified to know how to read quoted names...
	 *  also modified to look for a comma or eol token and then
	 *  try to give the maximal useful information when giving the
	 *  default "to" entry (e.g. "Dave Taylor <taylor@hpldat>"
	 *  will now give "Dave Taylor" rather than just "Dave")
	 */


	char            *address, 
			*bufptr, 
			mybuf[VERY_LONG_STRING];
	register int    indx = 0, 
			indx2 = 0;


	if ( equal(mail_to, username) )
		return;				/* can't be better! 		*/

	bufptr = (char *) buffer;		/* use the string directly   	*/

	if ( strchr(buffer, '"') != NULL ) {	/* we have a quoted string 	*/
		while ( buffer[indx] != '"' )
			indx++;

		indx++;			/* skip the leading quote 	*/
		
		while ( buffer[indx] != '"' && indx < strlen(buffer) )
			mail_to[indx2++] = buffer[indx++];

		mail_to[indx2] = '\0';
	} else {

		while ( (address = strtok(bufptr, ",\t\n\r")) != NULL ) {

			if ( !okay_address(address, "don't match me!") ) {
				strcpy( mail_to, username );	/* it's to YOU! */
				return;

			} else if ( strlen(mail_to) == 0 ) {	/* it's SOMEthing! */

				/*
				 *  this next bit is kinda gory, but allows us to use the
		  		 *  existing routines to parse the address - by pretending
		  		 *  it's a From: line and going from there...
		  	         */

				/*
				 *  ensure it ain't too long
				 */

				if ( strlen(address) > (sizeof mybuf) - 7 )
					address[(sizeof mybuf) - 7] = '\0';	

				sprintf( mybuf, "From: %s", address );
				parse_arpa_from( mybuf, mail_to );
				
			}

			bufptr = (char *) NULL;			/* set to null */
		}
	}

	return;
}


int
strip_dup_addr( addr_list )

char 		*addr_list;

{

	/*
	 *  This routine strip the duplicated address in the address list
	 *  with its comment.
	 *  The criteria is only address part(regardless of comment part).
	 *  The latter one precedes one before.
	 *  NOTE: Before put the address list into this routine, it must be
	 *  processed through 'parse_rfc822' function.
	 */


	int	indx = 0,			/* for work		   */
		next_start = 0;			/* location of next search */
	char	buffer[VERY_LONG_STRING],	/* buffer for new list	   */
		word1[VERY_LONG_STRING],	/* word from addr_list     */
		word2[VERY_LONG_STRING];	/* next word from addr_list*/


	buffer[0] = '\0';

	while ( next_start != -1 ){

		if ( ! get_word( addr_list, &next_start, word1 ) ){
			if ( next_start != -1 ){		/* not last component */

				indx = next_start;

				while ( indx != -1 ) {
					if ( ! get_word(addr_list, &indx, word2) )
						if ( equal(word1, word2) ) {
							indx = 0;
							break;	/* same one */
						}
				}

				if ( indx == -1 ){		/* copy the unique one */

					indx = next_start;
					
					if ( strlen( buffer ) == 0 )
						strcpy( buffer, word1 );
					else {
						strcat( buffer, " " );
						strcat( buffer, word1 );
					}

					/* then copy tailing comment */

					while ( get_word( addr_list, &indx, word1 ) ){
						strcat( buffer, " " );
						strcat( buffer, word1 );

						next_start = indx;

						if ( indx == -1 )
							break;

					}

				}

			} else {				/* address and last   */
				sprintf( buffer, "%s%s%s%c", buffer,
					 strlen( word1 ) && strlen( buffer ) ? " ":"",
					 word1, '\0' );
			}
		}  
	}

	strcpy( addr_list, buffer );				/* exchange and done */

	return( 0 );
}
