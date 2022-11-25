/**			hdrconfg_b.c			**/

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
 *    This file contains the routines necessary to be able to modify
 *    the mail headers of messages on the way off the machine for 
 *    users that are in 'batch' mode (e.g. the system has no knowledge
 *    of the termtype or anything, so needs to use dump printf calls)
 *
 *    Headers currently supported for modification are:
 *
 *	To:
 *	Cc:
 *	Bcc:			(depending on #ifdef BCC_ALLOWED)
 *	Subject:
 *	Reply-To:
 *
 *	The next set are a function of whether we're running in X.400 
 *	compatibility mode or not...If we aren't, then we have:
 *
 *	   Expires:
 *	   Priority:
 *         In-Reply-To:
 *	   Action:
 *
 *	  <user defined>
 *
 *	Otherwise we have:
 *
 *	   Expires:
 *	   Importance:
 *	   Level-of-Sensitivity:	(really "Sensitivity")
 *	   Notification:
 *
 */


#include <signal.h>

#include "headers.h"


#ifdef NLS
# define NL_SETN  20
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


/*
 * these are all defined in the mailmsg file! 
 */

extern char     subject[SLEN], 
		in_reply_to[SLEN], 
		expires[SLEN], 
		action[SLEN], 
		priority[SLEN], 
		to[VERY_LONG_STRING], 
		cc[VERY_LONG_STRING], 
		reply_to[VERY_LONG_STRING], 
		expanded_to[VERY_LONG_STRING], 
		expanded_cc[VERY_LONG_STRING],
		expanded_reply_to[VERY_LONG_STRING], 
		user_defined_header[SLEN];


#ifdef ALLOW_BCC
extern char     bcc[VERY_LONG_STRING], 
		expanded_bcc[VERY_LONG_STRING];
#endif

int 
batch_header_editor()

{
	/*
	 *  Edit headers.  
	 */


	char            ch;
	int             first_time_through = 0;


	printf( catgets(nl_fd,NL_SETN,1,"\r\nElm Header Editor.  Please choose from the following:\n\r\n"));

reprompt:
	if ( first_time_through++ == 0 ) {


#ifdef ALLOW_BCC
		printf("   V)iew headers  T)o:  C)c:  B)cc:  S)ubject:  R)eply-To:  E)xpires:\r\n");
#else
		printf("   V)iew headers  T)o:  C)c:  S)ubject:  R)eply-To:  E)xpires:\r\n");
#endif


		printf( "   P)riority:  I)n-reply-to:  A)ction:  or  Q)uit editing\r\n\n" );
		printf( catgets(nl_fd,NL_SETN,2,"Choice? q%c"), BACKSPACE );
	} else
		printf(catgets(nl_fd,NL_SETN,3,"\r\nHeader to edit ('?' for choices) : q%c"), BACKSPACE);

	fflush( stdout );

#ifdef SIGWINCH
	while((ch = (char)tolower(ReadCh())) == NO_OP_COMMAND)
		;
#else
	ch = (char)tolower(ReadCh());
#endif

	switch ( ch ) {
	case 'v':
		printf( catgets(nl_fd,NL_SETN,4,"View Headers:\r\n\n") );

		if ( strlen(to) > 0 ) {
			printf( "To: " );
			if ( strlen(expanded_to) > 0 )
				printf( "%s\r\n",
			        	format_long(expanded_to, 4) );
			else if ( strlen(to) > 0 )
				printf( "%s\r\n", format_long(to, 4) );
			else
				printf( "\r\n" );
		}

		if ( strlen(cc) > 0 ) {
			printf( "Cc: " );
			if ( strlen(expanded_cc) > 0 )
				printf( "%s\r\n", format_long(expanded_cc, 4) );
			else if ( strlen(cc) > 0 )
				printf( "%s\r\n", format_long(cc, 4) );
			else
				printf( "\r\n" );
		}

#ifdef ALLOW_BCC
		if ( strlen(bcc) > 0 ) {
			printf( "Bcc: " );
			if ( strlen(expanded_bcc) > 0 )
				printf( "%s\r\n", format_long(expanded_bcc, 4) );
			else if ( strlen(bcc) > 0 )
				printf( "%s\r\n", format_long(bcc, 4) );
			else
				printf( "\r\n" );
		}
#endif


		if ( strlen(subject) > 0 )
			printf( "Subject: %s\r\n", subject );
		if ( strlen(reply_to) > 0 ) {
			printf( "Reply-To: " );
			if ( strlen(expanded_reply_to) > 0 )
				printf( "%s\r\n",
			        	format_long(expanded_reply_to, 10) );
			else if ( strlen(reply_to) > 0 )
				printf( "%s\r\n", format_long(reply_to, 10) );
			else
				printf( "\r\n" );
		}

		if ( strlen(expires) > 0 )
			printf( "Expires: %s\r\n", expires );

		if ( strlen(priority) > 0 )
			printf( "Priority: %s\r\n", priority );

		if ( strlen(in_reply_to) > 0 )
			printf( "In-Reply-To: %s\r\n", in_reply_to );

		if ( strlen(action) > 0 )
			printf( "Action: %s\r\n", action );

		printf( "\r\n" );

		break;

	case 't':
		printf( "To\r\n\nTo: " );

		if ( strlen( to ) > VERY_LONG_STRING )
			to[VERY_LONG_STRING - 1] = '\0';

		if ( optionally_enter(to, -1, -1, TRUE) == -1 )
			return;

		if ( build_address(to, expanded_to) )
			printf( "\rTo: %s\r\n", format_long(expanded_to, 4) );
		else
			printf( "\r\n" );

		break;

	case 'c':
		printf( "Cc\r\n\nCc: " );

		if ( strlen( cc ) > VERY_LONG_STRING )
			cc[VERY_LONG_STRING - 1] ='\0';

		if ( optionally_enter(cc, -1, -1, TRUE) == -1 )
			return;

		if ( build_address(cc, expanded_cc) ) {
			if ( strlen(expanded_to) + strlen(expanded_cc)
			    +strlen(expanded_bcc) > VERY_LONG_STRING ) {
				printf(catgets(nl_fd,NL_SETN,11,"\rToo many people. Copies ignored !\r\n"));
				cc[0] = '\0';
				expanded_cc[0] = '\0';
			} else
				printf( "\rCc: %s\r\n", format_long(expanded_cc, 4) );
		} else
			printf( "\r\n" );

		break;


#ifdef ALLOW_BCC
	case 'b':
		printf( "Bcc\r\n\nBcc: " );

		if ( strlen( bcc ) > VERY_LONG_STRING )
			bcc[VERY_LONG_STRING - 1] ='\0';

		if ( optionally_enter(bcc, -1, -1, TRUE) == -1 )
			return;

		if ( build_address(bcc, expanded_bcc) ) {
			if ( strlen(expanded_to) + strlen(expanded_cc)
			    +strlen(expanded_bcc) > VERY_LONG_STRING ) {
				printf(catgets(nl_fd,NL_SETN,12,"\rToo many people. Blind Copies ignored !\r\n"));
				bcc[0] = '\0';
				expanded_bcc[0] = '\0';
			} else
				printf( "\rBcc: %s\r\n", format_long(expanded_bcc, 5) );
		} else
			printf( "\r\n" );

		break;
#endif


	case 's':
		printf( "Subject\r\n\nSubject: " );

		if ( optionally_enter(subject, -1, -1, TRUE) == -1 )
			return;

		break;

	case 'r':
		printf( "Reply-To\r\n\nReply-To: " );

		if ( optionally_enter(reply_to, -1, -1, TRUE) == -1 )
			return;

		if ( build_address(reply_to, expanded_reply_to) )
			printf( "\rReply-To: %s\r\n", format_long(expanded_reply_to, 10) );
		else
			printf( "\r\n" );

		break;

	case 'e':
		printf( "Expires\r\n\nExpires: " );

		batch_enter_date( expires );

		break;

	case 'p':
		printf( "Priority\r\n\nPriority: " );

		if ( optionally_enter(priority, -1, -1, TRUE) == -1 )
			return;

		break;

	case 'i':
		printf( "In-Reply-To\r\n\nIn-Reply-To: " );

		if ( optionally_enter(in_reply_to, -1, -1, TRUE) == -1 )
			return;

		break;

	case 'a':
		printf( "Action\r\n\nAction: " );

		if ( optionally_enter(action, -1, -1, TRUE) == -1 )
			return;

		break;

	case '\n':
	case '\r':
	case 'q':
		printf( catgets(nl_fd,NL_SETN,5,"Quit Header Editor\r\n") );
		return;

	case '?':
		printf( catgets(nl_fd,NL_SETN,6,"Help\r\n\n") );

		first_time_through = 0;

		break;

#ifdef SIGWINCH
	case (char) NO_OP_COMMAND:
		break;
#endif

	default:
		printf( catgets(nl_fd,NL_SETN,7,"%c\r\nI don't understand that choice!\r\n"), ch );
		first_time_through = 0;
	}

	goto reprompt;
}


int
batch_enter_date( buffer )

	char           *buffer;

{
	/*
	 *  Enter the number of days this message is valid for, then
	 *  display the actual date of expiration.  This routine relies 
	 *  heavily on the routine 'days_ahead()' in the file date.c
	 */


	int             days;


	printf( catgets(nl_fd,NL_SETN,8,"How many days in the future should this message expire? ") );
	fflush( stdout );
	Raw( OFF );
	gets( buffer );
	Raw( ON );
	sscanf( buffer, "%d", &days );

	if ( days < 1 )
		printf( catgets(nl_fd,NL_SETN,9,"You can't send messages that have already expired!\r\n") );
	else if ( days > 56 )
		printf( catgets(nl_fd,NL_SETN,10,"The expiration date must be within eight weeks of today\r\n") );
	else {
		days_ahead( days, buffer );
		printf( "Expires on: %s\r\n", buffer );
	}
}
