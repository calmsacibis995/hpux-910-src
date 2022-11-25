/**			mailtime.c			**/

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
 *  This set of routines is used to figure out when the user last read
 *  their mail and to also figure out if a given message is new or not.
 */


#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <ctype.h>

#include "headers.h"


int 
resolve_received( entry )

	struct header_rec *entry;

{
	/*
	 *  Entry has the data for computing the time and date the 
	 *  message was received.  Fix it and return 
	 */

	switch ( (char)tolower((int)entry->month[0]) ) {

	case 'j':
		if ( (char)tolower((int)entry->month[1]) == 'a' )
			entry->received.month = JANUARY;
		else if ( (char)tolower((int)entry->month[2]) == 'n' )
			entry->received.month = JUNE;
		else
			entry->received.month = JULY;
		break;

	case 'f':
		entry->received.month = FEBRUARY;
		break;

	case 'm':
		if ( (char)tolower((int)entry->month[2]) == 'r' )
			entry->received.month = MARCH;
		else
			entry->received.month = MAY;
		break;

	case 'a':
		if ( (char)tolower((int)entry->month[1]) == 'p' )
			entry->received.month = APRIL;
		else
			entry->received.month = AUGUST;
		break;

	case 's':
		entry->received.month = SEPTEMBER;
		break;

	case 'o':
		entry->received.month = OCTOBER;
		break;

	case 'n':
		entry->received.month = NOVEMBER;
		break;

	case 'd':
		entry->received.month = DECEMBER;
		break;

	}

	sscanf( entry->day, "%d", &(entry->received.day) );

	sscanf( entry->year, "%d", &(entry->received.year) );
	if ( entry->received.year > 100 )
		entry->received.year -= 1900;

	sscanf( entry->time, "%d:%d", &(entry->received.hour),
	       &(entry->received.minute) );
}


int
get_mailtime()

{
	/*
	 *  Instantiate the values of the last_read_mail stat
	 *  variable based on the file access time/date of the
	 *  file mailtime_file.  IF the file doesn't exist,
	 *  then assume all mail is new. 
	 */
	 

	struct stat     buffer;
	struct tm      *timebuf;
	char            filename[LONG_FILE_NAME];


	sprintf( filename, "%s/%s", home, mailtime_file );

	if ( stat(filename, &buffer) == -1 ) {
		last_read_mail.month = 0;
		last_read_mail.day = 0;
		last_read_mail.year = 0;
		last_read_mail.hour = 0;
		last_read_mail.minute = 0;
	} else {					/* stat okay... */
		timebuf = (struct tm *) localtime( &(buffer.st_mtime) );

		last_read_mail.month = timebuf->tm_mon;
		last_read_mail.day = timebuf->tm_mday;
		last_read_mail.year = timebuf->tm_year;
		last_read_mail.hour = timebuf->tm_hour;
		last_read_mail.minute = timebuf->tm_min;
	}
}


int
update_mailtime()

{
	/*
	 *  This routine updates the last modified time of the 
	 *  .last_read_mail file in the users home directory.
	 *  If the file doesn't exist, it creates it!! 
	 */


	char            filename[LONG_FILE_NAME];



	sprintf( filename, "%s/%s", home, mailtime_file );

	if ( utime(filename, (char *) NULL) == -1 )	/* note no "S"  */

	/*
	 *  That's what I like about programming for BSD & USG - the easy
	 *  portability between 'em.  Especially the section 2 calls!! 
	 */

		(void) creat( filename, 0777 );
}


int 
new_msg( entry )

	struct header_rec entry;

{
	/*
	 *  Return true if the current message is NEW.  This can be
	 *  easily tested by seeing 1) if we're reading the incoming
	 *  mailbox and then, if so, 2) if the received_on_machine
	 *  date is more recent than the last_read_mail date.
	 */


	if ( mbox_specified != 0 )
		return ( FALSE );			/* not incoming */


	/*
	 *  Two tests - if received is OLDER than last read mail, then
	 *  immediately return FALSE.  If received is NEWER than last
	 *  read mail then immediately return TRUE 
	 */


	if ( entry.received.year < last_read_mail.year )
		return ( FALSE );

	if ( entry.received.year > last_read_mail.year )
		return ( TRUE );

	if ( entry.received.month < last_read_mail.month )
		return ( FALSE );

	if ( entry.received.month > last_read_mail.month )
		return ( TRUE );

	if ( entry.received.day < last_read_mail.day )
		return ( FALSE );

	if ( entry.received.day > last_read_mail.day )
		return ( TRUE );

	if ( entry.received.hour < last_read_mail.hour )
		return ( FALSE );

	if ( entry.received.hour > last_read_mail.hour )
		return ( TRUE );

	if ( entry.received.minute < last_read_mail.minute )
		return ( FALSE );

	return ( TRUE );
}
