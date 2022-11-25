/**			expires.c			**/

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
 *  This routine is written to deal with the Expires: header on the
 *  individual mail coming in.  What it does is to look at the date,
 *  compare it to todays date, then set the EXPIRED flag on the
 *  current message if it is true...
 * 
 */

#include <time.h>

#include "headers.h"


int 
process_expiration_date( date, msg_status )

	char           *date;
	int            *msg_status;

{
	/*
	 *  first step is to break down the date given into MM DD YY HH MM
	 *  format:  The possible formats for this field are, by example:
	 *
	 *	(1) Mon, 11 Jun 87
	 *	(2) Jun 11, 87
	 *	(3) 11 Jun, 87
	 *	(4) 11/06/87	<- ambiguous - will be ignored!!
	 *	(5) 870611HHMMZ	(this is the X.400 format)
	 *
	 *  The reason #4 is considered ambiguous will be made clear
	 *  if we consider a message to be expired on Jan 4, 88:
	 *	01/04/88	in the United States
 	 *	04/01/88	in Europe
	 *  so is the first field the month or the day?  Standard prob.
	 */


	struct tm       *timestruct, 
			*localtime();
	long            thetime;
	char		buffer[VERY_LONG_STRING],
	                word1[NLEN], 
			word2[NLEN], 
			word3[NLEN], 
			word4[NLEN];
	int		i;
	int		space = 0,
	                month = 0, 
			day = 0, 
			year = 0, 
			hour = 0, 
			minute = 0;


	no_ret( date );
	strcpy( buffer, date );

	word1[0] = '\0';
	word2[0] = '\0';
	word3[0] = '\0';
	word4[0] = '\0';

	for ( i = 0; i < strlen( buffer ); i++ )
		if ( buffer[i] == ' ' )
			space ++;

	if ( space == 3 )
		sscanf( buffer, "%s %s %s %s", word1, word2, word3, word4 );
	else if ( space == 2 )
		sscanf( buffer, "%s %s %s", word1, word2, word3 );
	else if ( space == 0 )
		strcpy( word1, buffer );
	else 
		return;

	if ( strlen(word2) == 0 ) {			/* we have form #5 or form #4 */
		if ( isdigit(word1[1]) && isdigit(word1[2]) )	/* form #5! */
			sscanf( word1, "%2d%2d%2d%2d%2d%*c",
			        &year, &month, &day, &hour, &minute );
	} else if ( strlen(word4) != 0 ) {			/* form #1 */
		month = month_number( word3 );
		day = atoi( word2 );
		year = atoi( word4 );
	} else if ( !isdigit(word1[0]) ) {			/* form #2 */
		month = month_number( word1 );
		day = atoi( word2 );
		year = atoi( word3 );
	} else {						/* form #3 */
		day = atoi( word1 );
		month = month_number( word2 );
		year = atoi( word3 );
	}

	if ( day == 0 || year == 0 )
		return;					/* we didn't get a valid date */

	/* localtime() below returns the # of years since 1900.
	 */
	if ( year > 1900 )
		year -= 1900;

	/*
	 *  next let's get the current time and date, please 
	 */

	thetime = time( (long *) 0 );

	timestruct = localtime( &thetime );

	/*
	 *  and compare 'em 
	 */

	if ( year > timestruct->tm_year )
		return;
	else if ( year < timestruct->tm_year )
		goto expire_message;

	if ( month -1 > timestruct->tm_mon )
		return;
	else if ( month < timestruct->tm_mon )
		goto expire_message;

	if ( day > timestruct->tm_mday )
		return;
	else if ( day < timestruct->tm_mday )
		goto expire_message;

	if ( hour > timestruct->tm_hour )
		return;
	else if ( hour < timestruct->tm_hour )
		goto expire_message;

	if ( minute > timestruct->tm_min )
		return;

expire_message:

	/*
	 *  it's EXPIRED!  
	 */

	(*msg_status) |= EXPIRED;
}
