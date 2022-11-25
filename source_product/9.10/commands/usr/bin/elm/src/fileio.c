/**			fileio.c			**/

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
 *  File I/O routines, including deletion from the mailbox! 
 */


#include <sys/types.h>
#include <sys/stat.h>

#include "headers.h"


#ifdef NLS
# define NL_SETN  17
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


int 
copy_message( prefix, dest_file, remove_header, remote_rcpt )

	char            *prefix;
	FILE            *dest_file;
	int             remove_header;
	char		*remote_rcpt;

{
	/*
	 *  Copy current message to destination file, with optional 'prefix' 
	 *  as the prefix for each line.  If remove_header is true, it will 
	 *  skip lines in the message until it finds the end of header line...
         *  then it will start copying into the file...  If remote_rcpt is
	 *  non-NULL then it will rewrite the initial "From " line with a
	 *  line of the form "Resent-To: remote_rcpt" (for remailing).
 	 *
	 *  If "forwarding" is true then it'll do some nice things to
	 *  ensure that the forwarded message looks pleasant (e.g. remove
	 *  stuff like ">From " lines and "Received:" lines, and prefix
	 *  the entire message with "Forwarded message:\n" etc etc)
	 */


	char            buffer[VERY_LONG_STRING];
	register int    ok = 1, 
			lines, 
			in_header = 1, 
			saved_copy = FALSE,
			first_line = TRUE, 
			need_read_status = TRUE,
			ignoring = FALSE;

	/*
	 *  get to the first line of the message desired 
	 */

	if ( fseek(mailfile, header_table[current - 1].offset, 0) == -1 ) {
		dprint( 1, (debugfile,
		    	"ERROR: Attempt to seek %d bytes into file failed (%s)",
			header_table[current - 1].offset, "copy_message") );

		error1( catgets(nl_fd,NL_SETN,1,"ELM [seek] failed trying to read %d bytes into file"),
		        header_table[current - 1].offset );
		return(1);
	}

	/*
	 * how many lines in message? 
	 */

	lines = header_table[current - 1].lines;

	/*
	 * now while not EOF & still in message... copy it! 
	 */

	dprint( 8, (debugfile, "copy-message: msg#=%d, lines=%d\n", current, lines) );

	while ( ok && lines-- ) {

		if ( ok = (int) (fgets(buffer, VERY_LONG_STRING, mailfile) != NULL) ) {
			if ( first_word(buffer, "Status:") )
				need_read_status = FALSE;
			
			if ( strlen(buffer) < 2  && in_header ) {
				if ( need_read_status && !remove_header
				                      && !remote_rcpt ) {
					if ( (header_table[current - 1].status & NEW) &&
					     keep_in_incoming ) {
						if ( fprintf(dest_file,
						     "%s%s\n", prefix,
						     "Status: O") == EOF ) {
							force_final_newline(dest_file);
        						dprint(1, (debugfile,
							"\nattempt to write (copy_message) failed: %s\n\r",
							error_name(errno)) );
							return( 1 );
						}

					} else {
						if ( fprintf(dest_file,
						     "%s%s\n", prefix,
						     "Status: RO") == EOF ) {
							force_final_newline(dest_file);
        						dprint(1, (debugfile,
							"\nattempt to write (copy_message) failed: %s\n\r",
							error_name(errno)) );
							return( 1 );
						}
					}
				}
				in_header = FALSE;
			}
			if ( !(remove_header && in_header) ) {
				if ( first_line && remote_rcpt ) {
					no_ret( buffer );
					if ( first_word(buffer, "From ") ){
						/* Replace the "From " line
						 * with a "Resent-To:" line.
						 */
						saved_copy = TRUE;
						if ( fprintf(dest_file, "Resent-To: %s\n", remote_rcpt) == EOF ) {
							force_final_newline(dest_file);
							dprint(1, (debugfile, 
								   "\nattempt to write (copy_message) failed: %s\n\r",
								   error_name(errno)) );
							return( 1 );
						}
					}

					first_line = FALSE;

				} else if ( remote_rcpt && saved_copy && first_word(buffer, "Date:") ) {
					if ( fprintf(dest_file, "%s", buffer) == EOF ) {
						force_final_newline(dest_file);
						dprint(1, (debugfile, 
							   "\nattempt to write (copy_message) failed: %s\n\r",
							   error_name(errno)) );
						return( 1 );
					}
					if ( fprintf(dest_file, "Resent-Date: %s\n",
						     get_arpa_date()) == EOF ) {
						force_final_newline(dest_file);
						dprint(1, (debugfile, 
							   "\nattempt to write (copy_message) failed: %s\n\r",
							   error_name(errno)) );
						return( 1 );
					}
					
				} else if ( in_header && first_word(buffer, "Status:") ) {
					if ( remote_rcpt )
						/* ignore it */ ;
					else if ( ((!in_string(buffer, "R") &&
					     !(header_table[current - 1].status & NEW))) ||
					     !keep_in_incoming ) {
						if ( fprintf(dest_file,
					     	"%s%s\n", prefix,
					     	"Status: RO") == EOF ) {
							force_final_newline(dest_file);
        						dprint(1, (debugfile,
							"\nattempt to write (copy_message) failed: %s\n\r",
							error_name(errno)) );
							return( 1 );
						}
					} else {
						if ( fprintf(dest_file,
					     	"%s%s", prefix, buffer) == EOF ) {
							force_final_newline(dest_file);
        						dprint(1, (debugfile,
							"\nattempt to write (copy_message) failed: %s\n\r",
							error_name(errno)) );
							return( 1 );
						}
					}

				} else if ( !in_header && real_from(buffer, (struct header_rec *)0) ) {
					dprint( 1, (debugfile,
						   "\n*** Internal Problem...Tried to add the following;\n") );
					dprint( 1, (debugfile,
						   "  '%s'\nto output file (copy_message) ***\n",
						   buffer) );
					ok = 0;			/* stop now since we're
						 	 	 * presumably done... */
				} else if ( !in_header || !forwarding ){
					if ( fprintf(dest_file, "%s%s", prefix, buffer) 
									== EOF ){
						force_final_newline( dest_file );
						dprint(1, (debugfile, 
						    "\nattempt to write (copy_message) failed: %s\n\r",
					 	    error_name(errno)) );
						return( 1 );
					}

				} else if ( forwarding ) {
					if ( first_line ) {
						first_line = FALSE;
						if (fprintf( dest_file, 
							"Forwarded message:\n\n") == EOF ){
							force_final_newline( dest_file );
							dprint( 1, (debugfile, 
							   "\nattempt to write (copy_message) failed: %s\n\r",
							   error_name(errno)) );
							return( 1 );
						}
					} else if ( first_word(buffer, "Received:") ||
						    first_word(buffer, ">From") ||
			        		    first_word(buffer, "Return-Path:") )
						ignoring = TRUE;

					else if ( ignoring && !whitespace(buffer[0]) ) {
						ignoring = FALSE;
						if ( fprintf(dest_file, 
							"%s%s", prefix, buffer) == EOF ){
							force_final_newline( dest_file );
							dprint( 1, (debugfile,
							    "\nattempt to write (copy_message) failed: %s\n\r",
							    error_name(errno)) );
							return( 1 );
						}

					} else if ( !ignoring || !in_header ){
						if ( fprintf( dest_file, "%s%s",
							prefix, buffer) == EOF ){
							force_final_newline( dest_file );
							dprint( 1, (debugfile, 
							    "\nattempt to write (copy_message) failed: %s\n\r",
							    error_name(errno)) );
							return( 1 );
						}
					}	
				}
			}
		}
	}

	if ( strlen(buffer) + strlen(prefix) > 1 )
		if ( fprintf(dest_file, "\n") == EOF ){
			force_final_newline( dest_file );
			dprint( 1, (debugfile, 
				"\nattempt to write (copy_message) failed: %s\n\r",
				error_name(errno)) );
			return( 1 );
		}				/* blank line to keep
						 * mailx happy *sigh* */
	return( 0 );
}


/*
 *  the following routines are for a nice clean way to preserve
 *  the stats related to the file we're currently reading and all 
 */

static struct stat statbuff;


int 
save_file_stats( fname )

	char           *fname;

{

	if ( stat(fname, &statbuff) == -1 ){
		statbuff.st_mode = (unsigned short)0660;
		statbuff.st_uid = (unsigned short)geteuid();
		statbuff.st_gid = (unsigned short)getegid();
	}

	dprint( 2, (debugfile, "** saved stats for file %s **\n", fname) );
	return( 0 );
}


int
restore_file_stats( fname )

	char           *fname;

{
	/*
	 *  restore the file mode, but set our umask to 0 first to
	 *  ensure that doesn't get in the way... 
	 */


	int             old_umask;


	old_umask = umask( 0 );
	(void) chmod( fname, statbuff.st_mode & 0777 );
	(void) chown( fname, statbuff.st_uid, statbuff.st_gid );
	(void) umask( old_umask );
}


/*
 *  and finally, here's something for that evil trick: site hiding 
 */


#ifdef SITE_HIDING

int
is_a_hidden_user( specific_username )

	char           *specific_username;

{
	/*
	 *  Returns true iff the username is present in the list of
	 * 'hidden users' on the system.
	 *  this line is deliberately inserted to ensure that you THINK
	 *  about what you're doing, and perhaps even contact the author
	 *  of Elm before you USE this option...
	 */


	FILE           *hidden_users;
	char            buffer[SLEN];


	if ( (hidden_users = fopen(HIDDEN_SITE_USERS, "r")) == NULL ) {
		dprint( 1, (debugfile,
			   "Couldn't open hidden site file %s [%s]\n",
			   HIDDEN_SITE_USERS, error_name(errno)) );
		return ( FALSE );
	}

	while ( fscanf(hidden_users, "%s", buffer) != EOF )
		if ( equal(buffer, specific_username) ) {
			dprint( 3, (debugfile, "** Found user '%s' in hidden site file!\n",
				   specific_username) );
			fclose( hidden_users );
			return ( TRUE );
		}

	fclose( hidden_users );
	dprint( 3, (debugfile,
		   "** Couldn't find user '%s' in hidden site file!\n",
		   specific_username) );

	return ( FALSE );
}

#endif
