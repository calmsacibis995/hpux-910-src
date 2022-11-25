/**			calendar.c			**/

/*
 *  @(#) $Revision: 70.5 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  The current message can contain either of;
 *
 *	-> Mon 04/21 1:00p meet with chairman candidate
 *
 *  or 
 *
 *	- >April 21
 *	-   
 *	-     1:00 pm: Meet with Chairman Candidate
 *	-
 *
 *  The first type will have the leading '->' removed and all subsequent
 *  white space, creating a simple one-line entry in the users calendar
 *  file.   The second type will remove the '-' and the leading white
 *  spaces and leave everything else intact (that is, the file in the
 *  second example would be appended with ">April 21" followed by a
 *  blank line, the 1:00 pm meeting info, and another blank line.
 *
 *  The file to include this in is either the default as defined in the
 *  sysdefs.h file (see CALENDAR_FILE) or a filename contained in the
 *  users ".elm/elmrc" file, "calendar= <filename>".
 */

#include "headers.h"

#ifdef ENABLE_CALENDAR

/*
 * if not defined, this will be an empty file 
 */


#ifdef NLS
# define NL_SETN   7
# define nls_msg(i,s) ((char *)catgets(nl_fd, NL_SETN, i, s))
#else NLS
# define nls_msg(i,s) (s)
#endif NLS


int 
scan_calendar()

{
	FILE           *calendar;
	int             count;

	/*
	 * First step is to open the celendar file for appending... 
	 */

	 if ( (calendar = user_fopen(calendar_file, "a")) == NULL ) {

		int             err = errno;

		dprint( 2, (debugfile,
		        "Error: couldn't append to calendar file %s (scan)\n",
			calendar_file) );
		dprint( 2, (debugfile, "** %s - %s **\n",
			error_name(err), error_description(err)) );

		error2( nls_msg(1,"Couldn't append to file %s: %s"), calendar_file,
		        error_description(err) );
		return( 1 );
	}

	count = extract_info( calendar );

	user_fclose( calendar );

	chown( calendar_file, userid, groupid );	/* ensure owned by user */

	if ( count > 0 )
		error2( nls_msg(2,"%d entr%s saved in calendar file"),
		        count, count > 1 ? nls_msg(3,"ies") : nls_msg(4,"y") );
	else if ( count == 0 )
		error( nls_msg(5,"No calendar entries found in that message") );
	else
	        error1( nls_msg(6,"Sorry, couldn't append to calendar file: %s"),
			error_description(-count) );

	return( 0 );
}


int
extract_info( save_to_fd )

	FILE           *save_to_fd;

{
	/*
	 *  Save the relevant parts of the current message to the given
	 *  calendar file.  The only parameter is an opened file
	 *  descriptor, positioned at the end of the existing file 
	 */


	register int    entries = 0, 
			ok = 1, 
			lines, 
			indx, 
			in_entry = FALSE;
	char            buffer[VERY_LONG_STRING];


	/*
	 *  get to the first line of the message desired 
	 */

	if ( fseek(mailfile, header_table[current - 1].offset, 0) == -1 ) {
		dprint( 1, (debugfile,
		        "ERROR: Attempt to seek %d bytes into file failed (%s)",
			header_table[current - 1].offset, "extract_info") );

		error1( nls_msg(7,"ELM [seek] failed trying to read %d bytes into file"),
		        header_table[current - 1].offset );
		return ( 0 );
	}

	/*
	 * how many lines in message? 
	 */

	lines = header_table[current - 1].lines;

	/*
	 * now while not EOF & still in message... scan it! 
	 */

	while ( ok && lines-- ) {
		ok = (int) (fgets(buffer, VERY_LONG_STRING, mailfile) != NULL);

		/*
		 * now let's see if it matches the basic pattern... 
		 */

		if ( (indx = calendar_line(buffer) ) > -1) {

			if ( buffer[indx] == '>' ) {	/* single line entry */
				if ( remove_through_ch(buffer, '>') ) {
					if ( fprintf(save_to_fd, "%s", buffer) == EOF )
					        return( -errno );
					entries++;
				}

			} else {			/* multi-line entry  */
				if ( fprintf(save_to_fd, "%s", 
					     (char *) (buffer + indx + 1)) == EOF )
				        return( -errno );

				in_entry = TRUE;
			}

		} else if ( in_entry ) {
			in_entry = FALSE;
			entries++;
		}
	}

	dprint( 4, (debugfile,
	  "Got %d calender entr%s.\n", entries, entries > 1 ? "ies" : "y") );

	return ( entries );
}


int
calendar_line( string )

	char           *string;

{
	/*
	 *  Iff the input line is of the form;
 	 * 
	 *    {white space} <one or more '-'> 
	 *
	 *  this routine will return the index of the NEXT character
	 *  after the dashed sequence...If this pattern doesn't occur, 
	 *  or if any other problems are encountered, it'll return "-1"
	 */


	register int    loc = 0;


	if ( chloc(string, '-') == -1 )		/* no dash??? */
		return (-1);	

	/*
	 *  skip leading white space... 
	 */

	while ( whitespace(string[loc]) )
		loc++;				/* MUST have '-' too! */

	if ( string[loc] != '-' )
		return ( -1 );	

	while ( string[loc] == '-' )
		loc++;

	if ( loc >= strlen(string) )
		return ( -1 );			/* Empty line... */

	return ( loc );
}


int
remove_through_ch( string, ch )

	char            *string,
	                ch;

{
	/*
	 *  removes all characters from zero to ch in the string, and 
	 *  any 'white-space' following the 'n'th char... if it hits a
    	 *  NULL string, it returns FALSE, otherwise it'll return TRUE!
	 */


	char            buffer[VERY_LONG_STRING];
	register int    indx = 0, 
			i = 0;


	while ( string[indx] != ch && string[indx] != '\0' )
		indx++;

	if ( indx >= strlen(string) )
		return (FALSE);			/* crash! burn! */

	indx++;				/* get past the 'ch' character... */

	while ( whitespace(string[indx]) )
		indx++;

	while ( indx < strlen(string) )
		buffer[i++] = string[indx++];

	buffer[i] = '\0';

	strcpy( string, buffer );

	return ( TRUE );
}

#endif
