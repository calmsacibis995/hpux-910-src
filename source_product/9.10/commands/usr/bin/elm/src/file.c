/**			file.c				**/

/*
 *  @(#) $Revision: 72.2 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  File I/O routines, mostly the save to file command...
 *
 */

#include <stdio.h>
#include <signal.h>
#include <termio.h>
#include <sys/stat.h>				/* for umask() */

#include "headers.h"


#ifdef NLS
# define NL_SETN   15
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


extern int		num_char;

int
save( redraw, silently )

	int             *redraw, 
			silently;

{
	/*
	 *  Save all tagged messages + current in a file.  If no messages
	 *  are tagged, save the current message instead!  This routine
	 *  will return ZERO if the operation failed. 'redraw' is set to 
	 *  TRUE iff we use the '?' and mess up the screen.  If "silently"
	 *  is set, then don't output the "D" character upon marking for
	 *  deletion...
	 */


	register int    tagged = 0, 
			i, 
			oldstat, 
			appending = 0,
			iterations = 0, 
			l = 0,
			continue_looping;
	int		current_mask = 0,
			mask = 0117;
	char            filename[LONG_FILE_NAME], 
			address[VERY_LONG_STRING], 
			buffer[LONG_FILE_NAME];
	char		*buff,
			*strp;
	FILE            *fpipe,
			*save_file;
#ifdef SIGWINCH
	struct winsize	w_before, w_after;
	void		(*wstat) ();
#endif


	oldstat = header_table[current - 1].status;			/* remember */

	for ( i = 0; i < message_count; i++ )
		if ( ison(header_table[i].status, TAGGED) )
			tagged++;

	if ( tagged == 0 ) {
		tagged = 1;
		setit( header_table[current - 1].status, TAGGED );
	}

	dprint( 4, (debugfile, "%d message%s tagged for saving (save)\n", tagged,
		    plural(tagged)) );

	do {

		continue_looping = 0;			/* clear the flag, ho hum... */

		if ( !iterations && hp_terminal )
			define_softkeys( SAVE );

		if ( iterations++ ) {
			printf( catgets(nl_fd,NL_SETN,1,"File message(s) in: ") );
			fflush( stdout );
		} else
			PutLine1( LINES - 2, 0, catgets(nl_fd,NL_SETN,1,"File message(s) in: ") );

		if ( save_by_name ) {

			/*
			 *  build default filename to save to 
			 */

			get_return( address, FALSE );
			get_return_name( address, buffer, TRUE );
			buff = buffer;
			if ((strp = strchr(buff, '/')) != NULL)  {
				*strp = '\0';
				while(*--strp == '_' && l < 3) {
					l++;
					*strp = '\0';
				}
			}
			sprintf( filename, "=/%s", buffer );
		} else
			filename[0] = '\0';

		num_char = strlen(catgets(nl_fd,NL_SETN,1,"File message(s) in: ") );

		if ( iterations > 1 ) {
			optionally_enter( filename, -1, -1, FALSE );
		} else 
				optionally_enter( filename, LINES - 2, num_char, FALSE );

		if ( iterations == 1 )
			MoveCursor( LINES - 1, 0 );

		if ( strlen(filename) == 0 ) {	/** <return> means 'cancel', right? **/
			header_table[current - 1].status = oldstat;	/* BACK! */
			return ( 0 );
		}

		if ( equal(filename, "?") ) {
			*redraw = TRUE;		/* set the flag so we know what to do
					 	 * later */
			Raw( OFF );
			list_folders();
			Raw( ON );
			continue_looping++;
		}

	} while ( continue_looping );

	strp = filename;
	remove_spaces( strp );
	strcpy( filename, strp );
			
	if ( ((chloc(filename, ' ') != -1)   ||
	      (chloc(filename, '\t') != -1)) &&
	      (chloc(filename, '\'') == -1)  &&
	      (chloc(filename, '\"') == -1)     ) {
		error( catgets(nl_fd,NL_SETN,10,"Invalid file name.  Message is not saved.") );
		header_table[current - 1].status = oldstat;
		return ( 0 );
	}

	if ( (filename[0] == '\''                     && 
	      filename[strlen(filename) - 1] != '\'') ||
	     (filename[0] == '\"'                     &&
	      filename[strlen(filename) - 1] != '\"')    ) {
		error( catgets(nl_fd,NL_SETN,10,"Invalid file name.  Message is not saved.") );
		header_table[current - 1].status = oldstat;
		return ( 0 );
	}

#ifdef SIGWINCH
	ioctl(0, TIOCGWINSZ, &w_before);
#endif
#ifdef V4FS
	sprintf( buffer, "/usr/bin/echo %s", filename );
#else /* V4FS */
	sprintf( buffer, "/bin/echo %s", filename );
#endif /* V4FS */
	if ( (fpipe = popen(buffer, "r")) == NULL ) {
		error( catgets(nl_fd,NL_SETN,11,"Popen error.  Message is not saved.") );
		header_table[current - 1].status = oldstat;
		return ( 0 );
	}
#ifdef SIGWINCH
	wstat = signal(SIGWINCH, SIG_IGN);
#endif
	fgets( filename, LONG_FILE_NAME, fpipe ); 
	pclose( fpipe );
#ifdef SIGWINCH
	ioctl(0, TIOCGWINSZ, &w_after);
	if ((w_before.ws_row != w_after.ws_row) ||
			(w_before.ws_col != w_after.ws_col))
		setsize(w_after.ws_row, w_after.ws_col);
	signal( SIGWINCH, wstat );
#endif
        no_ret( filename );

	if ( equal(filename, "!") )
		sprintf( filename, "%s%s", mailhome, username );

	else if ( !expand_filename(filename) ) {

		dprint( 2, (debugfile,
		        "Error: Failed on expansion of filename '%s' (save)\n",
			filename));

		header_table[current - 1].status = oldstat;	/* BACK! */
		return ( 0 );			/* failed expanding name! */
	}

	if ( access(filename, ACCESS_EXISTS) == 0 )	/* already there!! */
		appending = 1;

	sprintf( buffer, "%s%s", mailhome, username );

	if ( equal( filename, buffer ) ) {

		if ( mbox_specified )
			(void)setresgid( -1, egroupid, egroupid );

		current_mask = umask( mask );
		save_file = fopen( filename, "a" );	/* fopen with gid = mail */
	} else
		save_file = user_fopen(filename, "a");	/* fopen with gid = rgid */
		
	if ( save_file == NULL) {

		int             err = errno;

		dprint( 2, (debugfile,
		        "Error: couldn't append to specified file '%s' (save)\n",
			filename) );
		dprint( 2, (debugfile, "** %s - %s **\n",
			error_name(err), error_description(err)) );

		set_error2( catgets(nl_fd,NL_SETN,3,"Couldn't append to '%s': %s"), filename,
		       error_description(err) );

		header_table[current - 1].status = oldstat;	/* BACK! */
		return ( 0 );
	}

	dprint( 4, (debugfile, "Saving mail to file '%s'...\n", filename) );

	for ( i = 0; i < message_count; i++ )		/* save each tagged msg */
		if ( header_table[i].status & TAGGED )
			if ( save_message(i, filename, save_file, (tagged > 1), 
					  appending++, silently) ) {
				set_error2( catgets(nl_fd,NL_SETN,4,"Couldn't append to '%s' completely: %s"), 
				        filename, error_description(errno) );

				if ( equal(filename, buffer) ) {
					fclose( save_file );
					(void) umask( current_mask );
				} else
					user_fclose( save_file );

				if ( first_word(filename, mailhome) ) {
					chown( filename, userid, getegid() );
					if ( mbox_specified )
						setresgid( -1, groupid, egroupid );
				} else
					chown( filename, userid, groupid );
				return ( 0 );
				
			}

	if ( equal(filename, buffer) ) {
		fclose( save_file );
		(void) umask( current_mask );
	} else
		user_fclose( save_file );

	if ( first_word(filename, mailhome) ) {
		chown( filename, userid, getegid() );

		if ( mbox_specified )
			(void)setresgid( -1, groupid, egroupid );
	} else
		chown( filename, userid, groupid );

	if ( tagged > 1 )
		set_error( catgets(nl_fd,NL_SETN,5,"Message(s) saved") );

	return ( 1 );
}


int
save_message( number, filename, fd, pause_me, appending, silently )

	int             number, 
			pause_me,
			appending, 
			silently;
	char            *filename;
	FILE            *fd;

{
	/*
	 *  Save an actual message to a file.  This is called by 
	 *  "save()" only!  The parameters are the message number,
	 *  and the name and file descriptor of the file to save to.
	 *  If 'pause_me' is true, a sleep(2) will be done after the
	 *  saved message appears on the screen...
	 *  'appending' is only true if the file already exists 
	 */


	register int    num,
			save_current;


	dprint( 4, (debugfile, "\tSaving message %d to file...\n", number) );

	save_current = current;
	current = number + 1;

	if ( copy_message("", fd, FALSE, NULL) ){
	       current = save_current;
	       return( 1 );
	}       

	current = save_current;

	if ( resolve_mode )
		setit( header_table[number].status, DELETED );	/* deleted, but ...   */

	clearit( header_table[number].status, TAGGED );		/* not tagged anymore */
	clearit( header_table[number].status, NEW );		/* it's not new now!  */

	if ( selected )
		num = compute_visible(number);
	else
		num = number+1;

	if ( appending )
		error2( catgets(nl_fd,NL_SETN,6,"Message %d appended to file '%s'"), num, filename );
	else
		error2( catgets(nl_fd,NL_SETN,7,"Message %d saved to file '%s'"), num, filename );

	if ( !silently )
		show_new_status( number );			/*update screen,if needed*/

	if ( pause_me && !silently )
		(void) sleep( 1 );

	return( 0 );
}


int
expand_filename( filename )

	char           *filename;

{
	/*
	 *  Expands '~' and '=' to specified file names, also will try to 
	 *  expand shell variables if encountered.. '+' and '%' are synonymous 
	 *  with '=' (folder dir)... 
	 */


	char            buffer[LONG_FILE_NAME], 
			varname[LONG_FILE_NAME], 
			env_value[LONG_FILE_NAME];
	register int    i = 1, 
			indx = 0;

	/*
	 *  New stuff - make sure no illegal char as last 
	 */

	if ( lastch(filename) == '\n' || lastch(filename) == '\r' )
		lastch( filename ) = '\0';

	if ( filename[0] == '~' && 
		strlen(home)+strlen(filename)+1 < LONG_FILE_NAME ) {
		sprintf( buffer, "%s%s%s", home,
		  	 (filename[1] != '/' && lastch(folders) != '/') ? "/" : "",
			 (char *) filename + 1 );
		strcpy( filename, buffer );

	} else if ( filename[0] == '=' || filename[0] == '+' 
				       || filename[0] == '%' ){
		if ( strlen(folders) == 0 ) {
			dprint( 3, (debugfile,
				   "Error: maildir not defined - can't expand '%c' (%s)\n",
				   filename[0], "expand_filename") );

			set_error1( catgets(nl_fd,NL_SETN,8,"MAILDIR not defined.  Can't expand '%c'"),
			     filename );
			return ( 0 );
		}

		if ( strlen(buffer)+strlen(folders)+1 < LONG_FILE_NAME ) {
			sprintf( buffer, "%s%s%s", folders,
		  	 (filename[1] != '/' && lastch(folders) != '/') ? "/" : "",
			 (char *) filename + 1 );

			strcpy( filename, buffer );
		}

	} else if ( filename[0] == '$' ) {		/* env variable! */
		while ( isalnum(filename[i]) )
			varname[indx++] = filename[i++];

		varname[indx] = '\0';

		env_value[0] = '\0';		/* null string for strlen! */

		if ( getenv(varname) != NULL )
			strcpy( env_value, getenv(varname) );

		if ( strlen(env_value) == 0 ) {
			dprint( 3, (debugfile,
				   "Error: Can't expand environment variable $%s (%s)\n",
				   varname, "expand_filename") );
			set_error1( catgets(nl_fd,NL_SETN,9,"Don't know what the value of $%s is!"), 
			     varname );
			return ( 0 );
		}

		if ( strlen(env_value)+strlen(filename)+1 < LONG_FILE_NAME ) {
			sprintf( buffer, "%s%s%s", env_value,
			 (filename[i] != '/' && lastch(env_value) != '/') ? "/" : "",
			 (char *) filename + i );
			 
			strcpy( filename, buffer );
		}
	}

	return ( 1 );
}
