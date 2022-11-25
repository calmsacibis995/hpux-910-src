/**			remail.c			**/

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
 *  For those cases when you want to have a message continue along
 *  to another person in such a way as they end up receiving it with
 *  the return address the person YOU received the mail from.
 */

#include "headers.h"


#ifdef NLS
# define NL_SETN  33
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


extern char	to[VERY_LONG_STRING], 
		expanded_to[VERY_LONG_STRING];

int
remail()

{
	/*
	 *  remail a message... returns TRUE if new foot needed ...
	 */


	FILE           *mailfd;
	char		filename[SLEN], 
			buffer[VERY_LONG_STRING];


	to[0] = '\0';

	if ( hp_terminal )
		define_softkeys( CANCEL );

	to[0] ='\0';

	if ( !get_to(to, expanded_to) )
		return ( 0 );			/* cancelled by softkey */

	if ( strlen(to) == 0 )
		return ( 0 );

	display_to( expanded_to );

	sprintf( filename, "%s/%s%d", tmpdir, temp_file, getpid() );

	if ( (mailfd = fopen(filename, "w")) == NULL ) {
		dprint( 1, (debugfile, "couldn't open temp file %s! (remail)\n",
			   filename) );
		dprint( 1, (debugfile, "** %s - %s **\n", error_name(errno),
			   error_description(errno)) );
		sprintf( buffer,catgets(nl_fd,NL_SETN,1,"Sorry - couldn't open file %s for writing (%s)"),
			 error_name(errno) );
		set_error( buffer );
		return ( 1 );

	}

	/*
	 *  now let's copy the message into the newly opened
	 *  buffer... 
	 */

	if ( copy_message("", mailfd, FALSE, expanded_to) ){

	        dprint( 1,(debugfile, 
			   "couldn't append to tempfile %s ! (remail)\n", filename) );
		sprintf( buffer, catgets(nl_fd,NL_SETN,2,"Sorry - couldn't append to file %s: %s"), 
			        filename, error_description );
		set_error( buffer );
		fclose( mailfd );
		return( 1 );

	}	

	fclose( mailfd );

	/*
	 *  Got the messsage, now let's ensure the person really wants to 
	 *  remail it...
	 */

	ClearLine( LINES - 1 );
	ClearLine( LINES );
	PutLine1( LINES - 1, 0,
		 catgets(nl_fd,NL_SETN,3,"Are you sure you want to remail this message (y/n) ? y%c"),
		 BACKSPACE );
	fflush( stdin );
	fflush( stdout );

	if ( hp_terminal )
		define_softkeys( YESNO );

	if ( want_to((char *) NULL, 'y', TRUE) != 'y' ) {	/* another No... */
		set_error( catgets(nl_fd,NL_SETN,4,"Bounce of message cancelled") );
		return ( 1 );
	}

	if ( strlen(mailer) + strlen(expanded_to) + strlen(filename) + 21
		> VERY_LONG_STRING ) {
		set_error(catgets(nl_fd,NL_SETN,7,"too many addresses to bounce to - mail not sent !") );
		return( 1 );
	}

	sprintf( buffer, "%s %s %s < %s > /dev/null 2>&1", sendmail, smflags,
			 strip_parens(expanded_to), filename );

	PutLine0( LINES, 0, catgets(nl_fd,NL_SETN,5,"resending mail...") );

	(void) system_call( buffer, SH, (char **)0 );

	set_error( catgets(nl_fd,NL_SETN,6,"Mail resent") );

	return ( 1 );
}
