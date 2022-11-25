/**			mkhdrs.c			**/

/*
 *  @(#) $Revision: 66.2 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  This contains all the header generating routines for the ELM
 *  program.
 */


#include "headers.h"


#ifdef NLS
# define NL_SETN  27
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


extern char     in_reply_to[SLEN];

char           *strcpy();
unsigned long   sleep();
FILE		*user_fopen();


int 
generate_reply_to( msg )

	int             msg;

{
	/*
	 *  Generate an 'in-reply-to' message... 
	 */

	char            buffer[SLEN];


	if ( msg == -1 )				/* not a reply! */
		in_reply_to[0] = '\0';
	else {
		if ( chloc(header_table[msg].from, '!') != -1)
			tail_of(header_table[msg].from, buffer, FALSE );
		else
			strcpy( buffer, header_table[msg].from );

		sprintf( in_reply_to, "%s%sfrom \"%s\" at %s %s, %s %s",
			header_table[msg].messageid,
			strlen( header_table[msg].messageid ) ? "; " : "",
			buffer,
			header_table[msg].month,
			header_table[msg].day,
			header_table[msg].year,
			header_table[msg].time );
	}

}


int 
add_mailheaders( filedesc )

	FILE           *filedesc;

{
	/*
	 *  Add the users .mailheaders file if available.  Allow backquoting 
	 *  in the file, too, for fortunes, etc...*shudder*
	 */


	FILE            *fd;
	char            filename[LONG_FILE_NAME], 
			buffer[LONG_SLEN];


	sprintf( filename, "%s/%s", home, mailheaders );

	if ( (fd = user_fopen(filename, "r")) != NULL ) {
		while ( fgets(buffer, LONG_SLEN, fd) != NULL )
			if ( strlen(buffer) < 2 ) {
				dprint( 2, (debugfile,
					   "Strlen of line from .elmheaders is < 2 (write_header_info)") );
				if ( mail_only )
					printf( catgets(nl_fd,NL_SETN,1,"Warning: blank line in %s ignored!\r\n"), 
					        filename );
				else {
					error1( catgets(nl_fd,NL_SETN,2,"Warning: blank line in %s ignored!"), 
						filename );
					sleep( 2 );
				}
			} else if ( occurances_of(BACKQUOTE, buffer) >= 2 ){
				if ( expand_backquote(buffer, filedesc) ) {
					if ( mail_only )
				        	printf( catgets(nl_fd,NL_SETN,3,"Warning: fail to execute backquoted command in header\r\n") );
					else
				        	error( catgets(nl_fd,NL_SETN,4,"Warning: fail to execute backquoted command in header") );
				}
			} else
				if ( fprintf(filedesc, "%s", buffer) == EOF ) {
					user_fclose( fd );
				        return( 1 );
				}

		user_fclose( fd );
	}
	return( 0 );
}


int 
expand_backquote( buffer, filedesc )

	char           *buffer;
	FILE           *filedesc;

{
	/*
	 *  This routine is called with a line of the form:
	 *	Fieldname: `command`
	 *  and is expanded accordingly..
	 */


	FILE            *fd;
	char            command[SLEN], 
			command_buffer[SLEN], 
			fname[SLEN], 
			prefix[SLEN];
	register int    i, 
			j = 0;


	for ( i = 0; buffer[i] != BACKQUOTE; i++ )
		prefix[j++] = buffer[i];
	prefix[j] = '\0';

	j = 0;

	for ( i = chloc(buffer, BACKQUOTE) + 1; buffer[i] != BACKQUOTE; i++ )
		command[j++] = buffer[i];

	command[j] = '\0';

	sprintf( fname, "%s/%s%d", tmpdir, temp_print, getpid() );

	sprintf( command_buffer, "%s > %s 2>/dev/null", command, fname );

	(void) system_call( command_buffer, SH, 0L );

	if ( (fd = fopen(fname, "r")) == NULL ) {
		if ( mail_only )
			printf( catgets(nl_fd,NL_SETN,5,"backquoted command \"%s\" in .elmheaders failed\r\n"),
			        command );
		else
			error1( catgets(nl_fd,NL_SETN,6,"backquoted command \"%s\" in .elmheaders failed"),
				command );
		return( 1 );
	}

	/*
	 * If we get a line that is less than 80 - length of prefix then we
	 * can toss it on the same line, otherwise, simply prepend each line
	 * starting with this line* with a leading tab and cruise along 
	 */

	if ( fgets(command_buffer, SLEN, fd) == NULL ){
		if ( fprintf(filedesc, prefix) == EOF ) {
		        fclose( fd );
			unlink( fname );
		        return( 1 );
		}	
	} else {
		if ( strlen(command_buffer) + strlen(prefix) < 80 ){
			if ( fprintf(filedesc, "%s%s", prefix, command_buffer) == EOF ) {
			        fclose( fd );
			        unlink( fname );
			        return( 1 );
			}	
		} else {
			if ( fprintf(filedesc,"%s\n\t%s", prefix, command_buffer) == EOF ){
			        fclose( fd );
				unlink( fname );
			        return( 1 );
			}
		}	

		while ( fgets(command_buffer, SLEN, fd) != NULL )
			if ( fprintf(filedesc, "\t%s", command_buffer)== EOF ) {
			        fclose( fd );
				unlink( fname );
			        return( 1 );
			}	

		fclose( fd );
	}

	unlink( fname );		/* don't leave the temp file laying around! */

	return( 0 );

}
