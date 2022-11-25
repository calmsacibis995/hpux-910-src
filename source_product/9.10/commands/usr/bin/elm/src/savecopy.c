/** 			savecopy.c			**/

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
 *  Save a copy of the specified message in the users savemail mailbox.
 */

#include <time.h>

#include "headers.h"

#define  metachar(c)	( c == '+' || c == '%' || c == '=' )


#ifdef NLS
# define NL_SETN  37
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


extern char     in_reply_to[SLEN];	/* In-Reply-To: string */


#ifdef ENCRYPTION_SUPPORTED
extern int      gotten_key;		/* for encryption      */
#endif

int
#ifdef ALLOW_BCC
save_copy( subject, to, cc, bcc, filename, original_to )
#else
save_copy( subject, to, cc, filename, original_to )
#endif

	char            *subject, 
			*to, 
			*cc, 
#ifdef ALLOW_BCC
			*bcc,
#endif
			*filename, 
			*original_to;

{
	/*
	 *  This routine appends a copy of the outgoing message to the
	 *  file specified by the SAVEFILE environment variable.  
	 */


	FILE            *save,			/* file id for file to save to */
	                *message;		/* the actual message body     */
	long            thetime,		/* variable holder for time    */
	                time();
	char            buffer[VERY_LONG_STRING],/* read buffer 		       */
	                savename[VERY_LONG_STRING],/* name of file saving into */
	                newbuffer[VERY_LONG_STRING];/* first name in 'to' line     */
	char		*buff,
			*strp;

	register int    i;			/* for chopping 'to' line up   */
	int		l = 0;


#ifdef ENCRYPTION_SUPPORTED
	int             crypted = 0;		/* are we encrypting?          */
#endif


	savename[0] = '\0';

	if ( save_by_name ) {
		get_return_name( to, buffer, FALSE );

		buff = buffer;
		if ((strp = strchr(buff, '/')) != NULL ) {
			*strp = '\0';
			while (*--strp == '_' && l < 3) {
				l++;
				*strp = '\0';
			}
		}
		if ( strlen(buffer) == 0 ) {
			dprint( 3, (debugfile,
				"Warning: get_return_name couldn't break down %s\n", to) );
			savename[0] = '\0';
		} else if ( strlen(folders)+strlen(buffer)+1 
				< VERY_LONG_STRING ) {
			sprintf( savename, "%s%s%s", folders,
				lastch(folders) == '/' ? "" : "/", buffer );
			if ( user_access(savename, ACCESS_EXISTS)!=0)
				savename[0] = '\0';
		} else 
			savename[0] = '\0';
	}

	if ( strlen(savename) == 0 ) {
		if ( strlen(savefile) == 0 ) {
			if ( mail_only )
				printf( catgets(nl_fd,NL_SETN,1,"variable 'SAVEFILE' not defined!\r\n") );
			else
				error( catgets(nl_fd,NL_SETN,2,"variable 'SAVEFILE' not defined!") );
			return ( FALSE );
		}

		if ( metachar(savefile[0]) &&
			strlen(folders)+strlen(savefile)+1 < VERY_LONG_STRING ) {
			sprintf( savename, "%s%s%s", folders,
			   lastch(folders) == '/' ? "" : "/", savefile + 1 );
		} else
			strcpy( savename, savefile );
	}

	if ( (save = user_fopen(savename, "a")) == NULL ) {
	
		int             err = errno;

		dprint( 1, (debugfile,
			 "Error: Couldn't append message to file %s (%s)\n",
			   savename, "save_copy") );
		dprint( 1, (debugfile, "** %s - %s **\n", error_name(err),
			   error_description(err)) );

		if ( mail_only )
			printf( catgets(nl_fd,NL_SETN,3,"couldn't append to %s : %s\r\n"), savename,
		       		error_description(err) );
		else
			error2( catgets(nl_fd,NL_SETN,4,"couldn't append to %s : %s"), savename,
		       		error_description(err) );

		(void) sleep( 2 );
		return ( FALSE );
	}

	if ( (message = fopen(filename, "r")) == NULL ) {

		int             err = errno;

		user_fclose( save );
		dprint( 1, (debugfile,
		   "Error: Couldn't read file %s (save_copy)\n", filename) );
		dprint( 1, (debugfile, "** %s - %s **\n", error_name(err),
			   error_description(err)) );

		if ( mail_only )
			printf( catgets(nl_fd,NL_SETN,5,"couldn't read file %s: %s\r\n"), 
				filename, error_description(err) );
		else
			error2( catgets(nl_fd,NL_SETN,6,"couldn't read file %s: %s"), 
				filename, error_description(err) );

		(void) sleep( 2 );
		return ( FALSE );
	}

	for ( i = 0; original_to[i] != '\0' && !whitespace(original_to[i]); i++ )
		newbuffer[i] = original_to[i];

	newbuffer[i] = '\0';

	tail_of( newbuffer, buffer, FALSE );

	thetime = time( (long *) 0 );

	if ( fprintf(save, "From To:%s %s", buffer, ctime(&thetime)) == EOF ){
	        force_final_newline( save );
	        fclose( message );
		user_fclose( save );
		return( FALSE );
	}	

	if ( fprintf(save, "Date: %s\n", get_arpa_date()) == EOF ){
	        force_final_newline( save );
	        fclose( message );
		user_fclose( save );
		return( FALSE );
	}	

	if ( fprintf(save, "To: %s\nSubject: %s\n",
		format_long(to, (int)strlen("To: ")), subject) == EOF ){
	        force_final_newline( save );
	        fclose( message );
		user_fclose( save );
		return( FALSE );
	}	

	if ( strlen(cc) > 0 )
		if ( fprintf(save, "Cc: %s\n",
			format_long(cc, (int)strlen("Cc:"))) == EOF ){
		        force_final_newline( save );
	                fclose( message );
		        user_fclose( save );
		        return( FALSE );
	        }	


#ifdef ALLOW_BCC
	if ( strlen(bcc) > 0 )
		if ( fprintf(save, "Bcc: %s\n",
			format_long(bcc, (int)strlen("Bcc:"))) == EOF ){
	                force_final_newline( save );
	                fclose( message );
		        user_fclose( save );
		        return( FALSE );
	        }	
#endif


	if ( strlen(in_reply_to) > 0 )
		if ( fprintf(save, "In-Reply-To: %s\n", in_reply_to) == EOF ){
	                force_final_newline( save );
	                fclose( message );
		        user_fclose( save );
		        return( FALSE );
	        }	

	if ( putc('\n', save) == EOF ){ /* put another return, please! */
	        fclose( message );
		user_fclose( save );
		return( FALSE );
	}
	
	/*
	 *  now copy over the message... 
	 */

	while ( fgets(buffer, VERY_LONG_STRING, message) != NULL ) {
		if ( buffer[0] == '[' ) {


#ifdef ENCRYPTION_SUPPORTED
			if ( first_word(buffer, START_ENCODE) )
				crypted = 1;
			else if ( first_word(buffer, END_ENCODE) )
				crypted = 0;
			else
#endif

				if ( first_word(buffer, DONT_SAVE) ||
				     first_word(buffer, DONT_SAVE2)  ) {

				/*
				 * second test added due to an
				 * imcompatability between the documentation
				 * and the software!  (Thanks Bill!) 
				 */

				fclose( message );
				user_fclose( save );
				chown( savename, userid, groupid );
				return ( TRUE );
			}
		}


#ifdef ENCRYPTION_SUPPORTED
		else if ( crypted ) {
			if ( !gotten_key++ )
				getkey( ON );
			encode( buffer );
		}
#endif


		if ( real_from(buffer, (struct header_rec *)0) ){
			if ( fprintf(save, ">%s", buffer) == EOF ){
	                        force_final_newline( save );
	                        fclose( message );
		                user_fclose( save );
		                return( FALSE );
	                }	
		} else {
			if ( fputs(buffer, save) == EOF ){
	                        force_final_newline( save );
	                        fclose( message );
		                user_fclose( save );
		                return( FALSE );
	                }
                }	
	}

	if ( fprintf(save, "\n") == EOF ){	/* ensure a blank line at the end */
                force_final_newline( save );
                fclose( message );
	        user_fclose( save );
	        return( FALSE );
        }	

	fclose( message );
	user_fclose( save );

	/* make sure save file isn't owned by root! */
	chown( savename, userid, groupid );

	return ( TRUE );
}
