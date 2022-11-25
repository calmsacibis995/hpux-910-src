/**			leavembox.c			**/

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
 *  leave current mailbox, updating etc. as needed...
 *
 */

#include <sys/types.h>
#include <sys/stat.h>

#include "headers.h"


#define  ECHOIT 	1	/* echo on for prompting! */


/*
 * Since a number of machines don't seem to bother to define the utimbuf
 * structure for some *very* obscure reason, you might need to pull it 
 * out of this comment...
 *
 * Suprise, though, BSD has a different utime() entirely...*sigh*
 *
 * struct utimbuf {
 *	time_t	actime;		/** access time       **	/ 
 *	time_t	modtime;	/** modification time ** 	/
 *      };
 */


#ifdef NLS
# include <nl_types.h>
# define NL_SETN  23
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


int
leave_mbox( quitting )

	int             quitting;

{
	/*
	 *  Exit, saving files into mbox and deleting specified, or simply 
	 *  delete specified mail... If "quitting" is true, then output status 
         *  regardless of what happens.  Returns 1 iff mailfile was
	 *  changed (ie messages deleted from file), 0 if not, -1 if new
	 *  mail has arrived in the meantime, and 2 if an alternate mailbox
	 *  was being read and was deleted.
	 */


	FILE           *temp;
	char            outfile[LONG_FILE_NAME], 
			buffer[SLEN];
	struct stat     buf;				/* stat command  */
	struct utimbuf  utime_buffer;			/* utime command */
	register int    to_delete = 0, 
			to_save = 0, 
			not_read = 0,
			newly_read = 0,
			i, 
			pending = 0, 
			number_saved = 0, 
			last_sortby;
	char            answer;
	long            bytes();


/*	Special fix for defect FSDlj08938
 *  Elm will corrupt your "/usr/mail/<login>" file
 *  These two variables should ALWAYS be FALSE when we get here,
 *  but somehow they aren't.  This brute force approach will fix that.
 */
	forwarding = FALSE;

	dprint( 1, (debugfile, "\n\n-- leaving_mailbox --\n\n") );

	if ( message_count == 0 ) {
		if ( mbox_specified == 0 )		/* null mailbox lock? */
			unlock();
		deleted_at_cancel = 0;
		saved_at_cancel   = 0;

		return ( FALSE );			/* nothing changed */
	}

	for ( i = 0; i < message_count; i++ ) {
		if ( ison(header_table[i].status, DELETED) )
			to_delete++;
		else
			to_save++;

		if ( header_table[i].status & NEW )
			not_read++;
		else if ( (header_table[i].status2 & NEW) ||
			  (header_table[i].status2 & URGENT) )
			newly_read++;
	}

	dprint( 2, (debugfile,
	       "Count: %d to delete and %d to save\n", to_delete, to_save) );

	if ( to_delete ) {

		if ( hp_terminal && question_me )
			define_softkeys( YESNO );	/* YES or NO on softkeys */
	
		if ( always_del )			/* set up the default answer... */
			answer = 'y';
		else
			answer = 'n';

		if ( to_save ) {
			if ( question_me ) {
				fflush( stdin );
				sprintf( buffer, catgets(nl_fd,NL_SETN,1,"Delete message(s)? (y/n) ") );
				answer = want_to( buffer, answer, ECHOIT );
			}

			if ( answer == eof_char ) {
				if ( hp_terminal )
					define_softkeys( MAIN );
				return( -1 );
			}

			if ( answer != 'y' ) {

				dprint( 3, (debugfile, 
					"\tDelete message%s? - answer was NO\n",
					plural(to_delete)) );
				if ( mbox_specified == 0 ) {
					to_save = message_count;
					to_delete = 0;
				} else {
					error( catgets(nl_fd,NL_SETN,2,"Nothing deleted") );
					return ( FALSE );	/* nothing was deleted! */
				}
			}

		} else if ( !to_save ) {		/* nothing to save!! */
			if ( question_me ) {
				fflush( stdin );
				sprintf( buffer,  catgets(nl_fd,NL_SETN,3,"Delete all message(s)? (y/n) ") );
				answer = want_to( buffer, answer, ECHOIT );
			}

			if ( answer == eof_char ) {
				if ( hp_terminal )
					define_softkeys( MAIN );
				return( -1 );
			}

			if ( answer != 'y' ) {

				dprint( 3, (debugfile, 
					"Delete all mail? - answer was NO\n") );
				if ( mbox_specified == 0 ) {
					to_save = message_count;
					to_delete = 0;
				} else {
					error( catgets(nl_fd,NL_SETN,4,"Nothing deleted") );
					return ( FALSE );	/* nothing was deleted */
				}
			}
		}
	}

	/*
	 *  we have to check to see what the sorting order was...so that
	 *  the order of saved messages is the same as the order of the
	 *  messages originally (a subtle point...) 
	 */

	if ( sortby != RECEIVED_DATE ) {		/* what we want anyway! */
		last_sortby = sortby;
		sortby = RECEIVED_DATE;
		sort_mailbox( message_count, 0, FALSE );
		sortby = last_sortby;
	}

	if ( to_save && mbox_specified == 0 ) {

		if ( hp_terminal && question_me )
			define_softkeys( YESNO );	/* YES or NO on softkeys */
	
		if ( always_leave )			/* set up default answer */
			answer = 'y';
		else
			answer = 'n';

		if ( !quitting && not_read )
			answer = 'y';

		if ( (question_me && (quitting || (!quitting && !not_read))) ) {
			fflush( stdin );
			answer = want_to( catgets(nl_fd,NL_SETN,5,"Keep mail in incoming mailbox? (y/n) "),
					  answer, ECHOIT );
		}

		if ( answer == eof_char ) {
			if ( hp_terminal )
				define_softkeys( MAIN );
			return( -1 );
		}

		if ( answer == 'y' ) {

			keep_in_incoming = TRUE;	/* mbox history */

			if ( to_delete || newly_read )	/* okay - keep undeleted as pending! */
				pending++;
			else {			/* nothing to delete, don't save */
				dprint( 3, (debugfile,
					"Keep mail in incoming mailbox? -- answer was YES\n") );
				if ( quitting )
					error( catgets(nl_fd,NL_SETN,6,"Mailbox unchanged") );
				if ( mbox_specified == 0 && quitting && !not_read )
					update_mailtime();
				
				if ( !newly_read )

					return ( FALSE );	/* nothing changed! */
			}

		} else
			keep_in_incoming = FALSE;		/* mbox histroy */
	}

	if ( !quitting ) {			/* To display a message at cancel   */
		deleted_at_cancel = to_delete;  /* in 'C)hange mbox' command.       */
		saved_at_cancel   = to_save;    /* These values are used in         */
	}

	if ( mbox_specified == 0 && quitting && !not_read )
		update_mailtime();


	/*
	 *  next, let's lock the file up and make one last size check
	 */

	if ( mbox_specified == 0 )
		lock( OUTGOING );

	if ( mailfile_size != bytes(infile) ) {
		unlock();
		error( catgets(nl_fd,NL_SETN,7,"New mail has just arrived - resyncing...") );
		return ( -1 );
	}

	/*
	 *  okay...now lets do it! 
	 */

	if ( to_save > 0 ) {
		if ( to_delete > 0 )
			sprintf( buffer, catgets(nl_fd,NL_SETN,8,"[keeping %d message(s), and deleting %d]"), to_save, to_delete );
		else if ( quitting )
			sprintf( buffer, catgets(nl_fd,NL_SETN,9,"[keeping all messages]") );
		else
			buffer[0] = '\0';		/* no string! */
	} else {
		if ( to_delete > 0 )
			sprintf( buffer, catgets(nl_fd,NL_SETN,15,"[deleting all messages]") );
		else if ( quitting )
			sprintf( buffer, 
				 catgets(nl_fd,NL_SETN,16,"[no messages to %s, and none to delete]"),
				 pending ? catgets(nl_fd,NL_SETN,17,"keep") : catgets(nl_fd,NL_SETN,18,"save") );
		else
			buffer[0] = '\0';
	}

	dprint( 2, (debugfile, "Action: %s\n", buffer) );

	error( buffer );

	if ( !mbox_specified ) {
		if ( pending ) { 			/* keep some messages pending! */
			sprintf( outfile, "%s/%s%d", tmpdir, temp_mbox, getpid() );
			unlink( outfile );
		} else if ( mailbox_defined )		/* save to specified mailbox */
			strcpy( outfile, mailbox );
		else					/* save to $home/mbox */
			sprintf( outfile, "%s/mbox", home );
	} else {
		if ( !to_delete && !newly_read )
			return ( FALSE );		/* no work to do! */
		sprintf( outfile, "%s/%s%d", tmpdir, temp_file, getpid() );
		unlink( outfile );			/* ensure it's empty! */
	}

	if ( to_save ) {

		if ( (temp = user_fopen(outfile, "a")) == NULL ) {
			int             err = errno;
			dprint( 1, (debugfile,
			      "Error: Couldn't append to outfile %s (%s)\n",
				   outfile, "leavembox") );
			dprint( 1, (debugfile, "** %s - %s **\n", error_name(err),
				   error_description(err)) );
			error2( catgets(nl_fd,NL_SETN,19,"Couldn't append to %s: %s\n"), outfile,
			       error_description(err) );
			if ( mbox_specified == 0 )
				unlock();
			return ( 0 );
		}

		for ( i = 0; i < message_count; i++ )
			if ( !(header_table[i].status & DELETED) 
			    			 || to_delete == 0 ) {
				current = i + 1;

				if ( !number_saved++ ) {
					dprint( 3, (debugfile, "Saving message%s #%d, ",
						 plural(to_save), current) );
				} else {
					dprint( 3, (debugfile, "#%d, ", current) );
				}

				if ( copy_message("", temp, FALSE, NULL) ){
				        user_fclose( temp );
					dprint( 1,(debugfile, 
						"leavembox: couldn't copy to %s - %s", 
						outfile, error_name(errno)) );
					unlink( outfile );
					unlock();
					error1( catgets(nl_fd,NL_SETN,20,"Mailbox %s unchanged: %s\n"),
						outfile, error_description(errno) );
					return( -1 );
				}	
			}

		user_fclose( temp );
		dprint( 2, (debugfile, "\n\n") );
	}

	/*
	 * remove source file...either default mailbox or original copy of
	 * specified one! 
	 */

	if ( stat(infile, &buf) != 0 ) {		/* grab original times... */

		int             err = errno;

		dprint( 1, (debugfile, "Error: errno %s attempting to stat file %s\n",
			   error_name(err), infile) );

		error3( catgets(nl_fd,NL_SETN,21,"Error %s (%s) on stat(%s)"), error_name(err),
		       error_description(err), infile );
	}

	fclose( mailfile );			/* close the baby... */

	if ( mailfile_size != bytes(infile) ) {
		sort_mailbox( message_count, 0, FALSE );	/* display sorting
							 * order! */
		unlock();
		error( catgets(nl_fd,NL_SETN,7,"New mail has just arrived - resyncing...") );
		return ( -1 );
	}


/* This next branch added for "/tmp full" on exit; 8th June, 1992
 * DTS# UCSqm00353 */

	if ( infile != NULL ) {
		if ((bytes(outfile) > 1) || ( !to_save)) {
			unlink( infile );
			}
		else if ((bytes(outfile) <1) && ( to_save )) {
			error1( catgets(nl_fd,28,19,"\nCouldn't open file %s for use as temp mailbox;\n"), outfile );
			emergency_exit();
			}
		}

	if ( to_save && (mbox_specified || pending) ) {
		if ( link(outfile, infile) != 0 )
			if ( errno == EXDEV ) {	/** different file devices!  Use copy! **/
				if ( copy(outfile, infile) != 0 ) {
					dprint( 1, (debugfile, 
						    "leavembox: copy(%s, %s) failed;",
						   outfile, infile) );
					dprint( 1, (debugfile, 
						    "** %s - %s **\n", error_name(errno),
						    error_description(errno)) );
					error( catgets(nl_fd,NL_SETN,23,"couldn't modify mail file!") );

					(void) sleep( 1 );
					sprintf( infile, "%s/%s", home, unedited_mail );

					if ( copy(outfile, infile) != 0 ) {
						dprint( 1, (debugfile,
							   "leavembox: couldn't copy to %s either!!  Help;",
							   infile) );
						dprint( 1, (debugfile, 
							"** %s - %s **\n",error_name(errno),
						 	error_description(errno)) );
						error( catgets(nl_fd,NL_SETN,24,"something terrible is happening to me!!!") );
						emergency_exit();
					} else {
						dprint( 1, (debugfile,
							   "\nWhoa! Confused - Saved mail in %s (leavembox)\n",
							   infile) );
						error1( catgets(nl_fd,NL_SETN,25,"saved mail in %s"), 
							infile );
					}
				}
			} else {
				dprint( 1, (debugfile, "link(%s, %s) failed (leavembox)\n",
					   outfile, infile) );
				dprint( 1, (debugfile, "** %s - %s **\n", error_name(errno),
					   error_description(errno)) );
				error2( catgets(nl_fd,NL_SETN,26,"link failed! %s - %s"), 
					error_name(errno), error_description(errno) );
				emergency_exit();
			}

		unlink( outfile );
		restore_file_stats( infile );

	} else if ( keep_empty_files ) {
		(void) sleep( 1 );
		error1( catgets(nl_fd,NL_SETN,27,"..keeping empty mail file '%s'.."), infile );
		temp = fopen( infile, "w" );
		fclose( temp );
		restore_file_stats( infile );
	} else if (mbox_specified)
		/*
		 * return an indication that mail file no longer exists.  if
		 * we are resyncing, this will cause us to default to standard
		 * mail file.
		 */
		return(2);
	
	if ( mbox_specified == 0 ) {

		if ( !pending && keep_empty_files ){ /* if none still being saved */
			temp = fopen( infile, "w" );
		 	fclose( temp );
		}

			restore_file_stats( infile );

		/*
		 * let's set the access times of the new mail file to be the
		 * same as the OLD one (still sitting in 'buf') ! 
		 */

		utime_buffer.actime = buf.st_atime;
		utime_buffer.modtime = buf.st_mtime;

		if ( utime(infile, &utime_buffer) != 0 ) {
			dprint( 1, (debugfile,
			   "Error: encountered error doing utime (leavmbox)\n") );
			dprint( 1, (debugfile, "** %s - %s **\n", error_name(errno),
			   error_description(errno)) );
		}

		unlock();			/* remove the lock on the file ASAP! */

		/*
		 *  finally, let's change the ownership of the default
      	     	 *  outgoing mailbox, if needed 
		 */

		if ( to_save )
			chown( outfile, userid, groupid );
	}

	return ((to_delete > 1) ? 1 : to_delete);
}


char            lock_name[SLEN];


int 
lock( direction )

	int             direction;

{
	/*
	 *  Create lock file to ensure that we don't get any mail 
	 *  while altering the mailbox contents!
	 *  If it already exists sit and spin until 
         *     either the lock file is removed...indicating new mail
	 *  or
	 *     we have iterated MAX_ATTEMPTS times, in which case we
	 *     either fail or remove it and make our own (determined
	 *     by if REMOVE_AT_LAST is defined in header file
	 *
	 *  If direction == INCOMING then DON'T remove the lock file
	 *  on the way out!  (It'd mess up whatever created it!).
	 */


	register int    iteration = 0, lock_fd;


	sprintf( lock_name, "%s%s.lock", mailhome, username );

	lock_fd = open( lock_name, O_WRONLY | O_CREAT | O_EXCL, 0777 );

	if ( lock_fd < 0 && errno == EACCES ) {
		if ( direction == OUTGOING ) {
			dprint( 1, (debugfile,
			 "Error encountered attempting to create lock %s\n",
				   lock_name) );
			dprint( 1, (debugfile, "** %s - %s **\n", error_name(errno),
				   error_description(errno)) );

			MoveCursor( LINES, 0 );

			printf( catgets(nl_fd,NL_SETN,29,"\n\rError encountered while attempting to create lock file %s;\n\r"),
			       lock_name );
			printf( "** %s - %s **\n\r\n\r", error_name(errno),
			       error_description(errno) );
			leave( errno );

		} else {		/* permission denied in the middle?  Odd... */
			dprint( 1, (debugfile,
				   "Can't create lock file: creat(%s) raises error %s (lock)\n",
				   lock_name, error_name(errno)) );
			error1( catgets(nl_fd,NL_SETN,30,"Can't create lock file!  I need write permission in \"%s\"\n\r"),
				     mailhome);
			leave( errno );
			
		}
	}

	/*
	 *  if lock_fd is not a valid file descriptor at this point, it
	 *  means that we're circling because someone *else* has the 	
   	 *  file descriptor.  We'll keep iterating until something 
	 *  new and exciting happens or we give up.
	 */

	while ( lock_fd < 0 && iteration++ < MAX_ATTEMPTS ) {

		dprint( 2, (debugfile, "File '%s' already exists!  Waiting...(lock)\n",
			   lock_name) );

		if ( direction == INCOMING ) {
			if ( iteration == 1 )
				PutLine0( LINES, 0, catgets(nl_fd,NL_SETN,31,"Mail being received...\t\twaiting...1 ") );
			else
				printf( "%d ", iteration );

			fflush( stdout );
		} else
			error1( catgets(nl_fd,NL_SETN,32,"  Attempt #%d: Mail being received...waiting  "), iteration );
		(void) sleep( 5 );

		lock_fd = open( lock_name, O_WRONLY | O_CREAT | O_EXCL, 0777 );
	}

	if ( lock_fd < 0 ) {			/* we tried.  We really did.  */


#ifdef REMOVE_AT_LAST

		/*
		 *  time to waste the lock file!  Must be there in error! 
		 */

		dprint( 2, (debugfile,
			   "Warning: I'm giving up waiting - removing lock file(lock)\n") );

		if ( direction == INCOMING )
			PutLine0( LINES, 0, catgets(nl_fd,NL_SETN,33,"\nTimed out - removing current lock file...") );
		else
			error( catgets(nl_fd,NL_SETN,34,"Throwing away the current lock file!") );

		if ( unlink(lock_name) != 0 ) {
			dprint( 1, (debugfile,
			 	   "Error %s (%s)\n\ttrying to unlink file %s (%s)\n",
				   error_name(errno), error_description(errno), lock_name));
			PutLine1( LINES, 0,

			catgets(nl_fd,NL_SETN,35,"\n\rI couldn't remove the current lock file %s\n\r"),
				 lock_name );
			PutLine2( LINES, 0, "** %s - %s **\n\r", error_name(errno),
				 error_description(errno) );

			if ( direction == INCOMING )
				leave( errno );
			else
				emergency_exit();
		}

		/*
		 * everything is okay, so lets act as if nothing had
		 * happened... 
		 */

#else

		/*
		 * Okay...we die and leave, not updating the mailfile mbox or
		 * any of those! 
		 */

		if ( direction == INCOMING ) {
			PutLine1( LINES, 0, catgets(nl_fd,NL_SETN,36,"\n\r\n\rGiving up after %d iterations...\n\r"),
				 iteration );
			PutLine0( LINES, 0, catgets(nl_fd,NL_SETN,37,"\n\rPlease try to read your mail again in a few minutes.\n\r\n\r") );
			dprint( 1, (debugfile,
				   "Warning: bailing out after %d iterations...(lock)\n",
				   iteration) );
			leave_locked( 0 );
		} else {
			dprint( 1, (debugfile,
			"Warning: after %d iterations, timed out! (lock)\n",
				   iteration) );
			
			error(catgets(nl_fd,NL_SETN,38,"Timed out on lock file reads.  Leaving program."));
			leave( 1 );
		}

#endif


	}

	/*
	 * if we get here we've created the lock file, so lets just split 
	 */

	close( lock_fd );		/* close it.  We don't want to KEEP the
				 	 * thing! */
}


int 
unlock()

{
	/*
	 *  Remove the lock file!    This must be part of the interrupt
	 *  processing routine to ensure that the lock file is NEVER
	 *  left sitting in the mailhome directory! 
	 */


	(void) unlink( lock_name );
}
