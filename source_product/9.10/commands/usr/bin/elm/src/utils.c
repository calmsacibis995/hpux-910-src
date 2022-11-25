/**			utils.c				**/

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
 *  Utility routines for ELM 
 *
 */

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "headers.h"

#ifdef NLS
#   define NL_SETN   45
#else NLS
#   define catgets(i,sn,mn,s) (s)
#endif NLS

#ifdef SIGWINCH
extern int  state_stack[];	/* elm states for SIGWINCH screen refresh */
extern int  sstack_p;		/* points to top of state_stack */
#endif

char		shortened_name[STRING];

void
move_old_files_to_new()
{
	/*
	 *  this routine is just for allowing people to transition from
	 *  the old Elm, where things are all kept in their $HOME dir,
	 *  to the new one where everything is in $HOME/.elm... 
	 */

	char		buffer[SLEN],
			source[SLEN], 
			dest[SLEN];

	/*
	 *  first off, let's make the directory we need... 
	 */

	sprintf( source, "%s/.elm", home );

	(void) mkdir( source, 0700 );

	/*
	 * and simply go through all the files... 
	 */

	sprintf( source, "%s/.alias_text", home );

	if ( access(source, ACCESS_EXISTS) != -1 ) {
		sprintf( dest, "%s/%s", home, ALIAS_TEXT );
		printf( "\n\r%s   -->   %s   \t[':' changed to '=']\n", source, dest );

		sprintf( buffer, "/bin/sed -e 's/:/=/g' %s > %s", source, dest );

		if ( system_call(buffer, SH, (char **)0) == 0 )
			unlink( source );

	}

	sprintf( source, "%s/.alias_data", home );

	if ( access(source, ACCESS_EXISTS) != -1 ) {
		sprintf( dest, "%s/%s", home, ALIAS_DATA );
		printf( "%s   -->   %s\n", source, dest );

		if ( movefile(source, dest) )
		        printf( "Fail to move: %s", error_description );
	}

	sprintf( source, "%s/.alias_hash", home );

	if ( access(source, ACCESS_EXISTS) != -1 ) {
		sprintf( dest, "%s/%s", home, ALIAS_HASH );
		printf( "%s   -->   %s\n", source, dest );

		if ( movefile(source, dest) )      
		        printf( "Fail to move: %s", error_description );
	}

	sprintf( source, "%s/.elmheaders", home );

	if ( access(source, ACCESS_EXISTS) != -1 ) {
		sprintf( dest, "%s/%s", home, mailheaders );
		printf( "%s   -->   %s\n", source, dest );

		if ( movefile(source, dest) )      
		        printf( "Fail to move: %s", error_description );

	}

	sprintf( source, "%s/.elmrc", home );

	if ( access(source, ACCESS_EXISTS) != -1 ) {
		sprintf( dest, "%s/%s", home, elmrcfile );
		printf( "%s   -->   %s\n", source, dest );

		if ( movefile(source, dest) )      
		        printf( "Fail to move: %s", error_description );
	}

	sprintf( source, "%s/.last_read_mail", home );

	if ( access(source, ACCESS_EXISTS) != -1 ) {
		sprintf( dest, "%s/%s", home, mailtime_file );
		printf( "%s   -->   %s\n", source, dest );

		if ( movefile(source, dest) )
		        printf( "Fail to move: %s", error_description );
	}

	printf( "\n\rWelcome to the New Elm system...\n\n\r" );
	(void) sleep( 3 );
}


void
show_mailfile_stats()
{
	/*
	 *  when we're about to die, let's try to dump lots of 
	 *  good stuff to the debug file... 
	 */

#ifdef DEBUG
	struct stat     buffer;
#endif

	if ( debug == 0 )
		return;		

#ifdef DEBUG
	if ( fstat(fileno(mailfile), &buffer) == 0 ) {
		dprint( 1, (debugfile, "\nDump of stats for mailfile %s;\n", infile) );

		dprint( 1, (debugfile, "\tinode: %d, mode: %o, uid: %d, ",
			   buffer.st_ino, buffer.st_mode, buffer.st_uid) );

		dprint( 1, (debugfile,
		   "gid: %d, size: %d\n\n", buffer.st_gid, buffer.st_size) );

		dprint( 1, (debugfile, "\toffset into file = %l\n", ftell(mailfile)) );
	}
	else
		dprint( 1, (debugfile,
			    "\nfstat on mailfile '%s' failed with error %s!!\n\n",
			    infile, error_name(errno)) );
#endif
}


void
emergency_exit()
{
	/*
	 *  used in dramatic cases when we must leave without altering
	 *  ANYTHING about the system... 
	 */

	dprint( 1, (debugfile,
		   "\nERROR: Something dreadful is happening!  Taking emergency exit!!\n\n") );
	dprint( 1, (debugfile,
		   "  possibly leaving behind the following files;\n") );
	dprint( 1, (debugfile,
		"     The mailbox tempfile : %s/%s%s\n", tmpdir, temp_mbox, username) );
	dprint( 1, (debugfile,
	    "     The mailbox lock file: %s%s.lock\n", mailhome, username) );
	dprint( 1, (debugfile,
		"     The composition file : %s/%s%d\n", tmpdir, temp_file, getpid()) );
	dprint( 1, (debugfile,
	    "     The header comp file : %s/%s%d\n", tmpdir, temp_head_file, getpid()) );
	dprint( 1, (debugfile,
		"     The readmail data file: %s/%s\n", home, readmail_file) );
	dprint( 1, (debugfile,
		"     The alias text file: %s/%s%d\n", tmpdir, temp_alias, getpid()) );

	Raw( OFF );
	if ( has_transmit )
		transmit_functions( OFF );

	if ( hp_terminal ) {
		define_softkeys(CLEAR);
		softkeys_off();
	}

	if ( cursor_control )
		MoveCursor( LINES, 0 );

	unlock();

	PutLine0( LINES, 0,
		 catgets(nl_fd,NL_SETN,1,"\nEmergency Exit taken!  All temp files intact!\n\n") );

#ifdef NLS
	(void) catclose( nl_fd );
#endif NLS

	exit( 1 );
}

void
leave( val )

	int             val;

{
	char            buffer[SLEN];

#ifdef DEBUG
	if ( debugfile != NULL )
		dprint( 2, (debugfile, "\nLeaving mailer normally (leave(%d))\n", val) );
#endif

	Raw( OFF );

	if ( has_transmit )
		transmit_functions( OFF );

	if ( hp_terminal ) {
		define_softkeys(CLEAR);
		softkeys_off();
	}

	/* editor buffers */
	(void) sprintf( buffer, "%s/%s%d", tmpdir, temp_file, getpid() );
	(void) unlink( buffer );
	(void) sprintf( buffer, "%s/%s%d", tmpdir, temp_head_file, getpid() );
	(void) unlink( buffer );

	if ( ! mail_only ) {
		if ( mbox_specified == 0 ) {
		    /* temp mailbox */
		    (void) sprintf( buffer, "%s/%s%s",
				    tmpdir, temp_mbox, username );
		    (void) unlink( buffer );
		}

		/* readmail temp */
		sprintf( buffer, "%s/%s", home, readmail_file );
		(void) unlink( buffer );

		unlock();

		MoveCursor( LINES, 0 );
		Writechar( '\n' );
	}

#ifdef NLS
	(void) catclose( nl_fd );
#endif NLS

	exit( val );
}


void
silently_exit()
{
	/*
	 *  This is the same as 'leave', but it doesn't remove any non-pid
	 *  files.  It's used when we notice that we're trying to create a
	 *  temp mail file and one already exists!!
	 */


	char            buffer[SLEN];


	dprint( 2, (debugfile, "\nLeaving mailer quietly (silently_exit)\n") );

	Raw( OFF );

	if ( has_transmit )
		transmit_functions( OFF );

	if ( hp_terminal )
		softkeys_off();

	sprintf( buffer, "%s/%s%d", tmpdir, temp_file, getpid() ); /* editor buffer */
	(void) unlink( buffer );

	sprintf( buffer, "%s/%s%d", tmpdir, temp_head_file, getpid() ); /* editor buffer */
	(void) unlink( buffer );

	if ( !mail_only ) {
		MoveCursor( LINES, 0 );
		Writechar( '\n' );
	}

#ifdef NLS
	(void) catclose( nl_fd );
#endif NLS

	exit( 0 );
}

/*ARGSUSED*/
void
leave_locked( val )

	int	val;	/* not used, placeholder for signal catching! */

{
	/*
	 * same as leave() routine, but don't disturb lock file
	 */


	char            buffer[SLEN];


	dprint( 3, (debugfile,
	        "\nLeaving mailer due to presence of lock file (leave_locked)\n") );

	Raw( OFF );

	if ( has_transmit )
		transmit_functions( OFF );
	if ( hp_terminal )
		softkeys_off();

	sprintf( buffer, "%s/%s%d", tmpdir, temp_file, getpid() ); /* editor buffer */
	(void) unlink( buffer );

	sprintf( buffer, "%s/%s%d", tmpdir, temp_head_file, getpid() );	/* editor buffer */
	(void) unlink( buffer );

	sprintf( buffer, "%s/%s%s", tmpdir, temp_mbox, username ); /* temp mailbox */
	(void) unlink( buffer );

	MoveCursor( LINES, 0 );
	Writechar( '\n' );

#ifdef NLS
	(void) catclose( nl_fd );
#endif NLS

	exit( 0 );
}


int
get_page( msg_pointer )

	int             msg_pointer;

{
	/*
	 *  Ensure that 'current' is on the displayed page,
	 *  returning non-zero iff the page changed! 
	 */


	register int    first_on_page, 
			last_on_page;


	first_on_page = ( header_page * headers_per_page ) + 1;

	last_on_page = first_on_page + headers_per_page - 1;

	if ( selected )			/* but what is it on the SCREEN??? */
		msg_pointer = compute_visible(msg_pointer - 1);

	if ( selected && msg_pointer > selected 
	     || !selected && msg_pointer > message_count )
		return ( FALSE );	/* too far - page can't change! */

	if ( msg_pointer > last_on_page ) {
		header_page = (int) ( msg_pointer - (selected ? 0 : 1) ) / headers_per_page;
		return ( 1 );

	} else if ( msg_pointer < first_on_page ) {
		header_page = (int) ( msg_pointer - 1 ) / headers_per_page;
		return ( 1 );
	} else
		return ( 0 );
}


char   *
nameof( filename )

	char           *filename;

{
	/*
	 *  checks to see if 'filename' has any common prefixes, if
	 *  so it returns a string that is the same filename, but 
	 *  with '=' as the folder directory, or '~' as the home
	 *  directory..
	 */


	register int    i = 0, 
			indx = 0;


	if ( first_word(filename, folders) ) {
		if ( strlen(folders) > 0 ) {
			shortened_name[i++] = '=';
			indx = strlen( folders );
		}
	} else if ( first_word(filename, home) ) {
		if ( strlen(home) > 0 ) {
			shortened_name[i++] = '~';
			indx = strlen( home );
		}
	} else
		indx = 0;

	while ( filename[indx] != '\0' && i < STRING )
		shortened_name[i++] = filename[indx++];

	shortened_name[i] = '\0';

	return ( (char *) shortened_name );
}

#ifdef SIGWINCH
/* routines to manage the stack that keeps track of the current elm state.  the
   state is used by the SIGWINCH signal handler to know how to redraw the
   screen. */

void
init_sstack()
{
	/* initialize the state stack to empty */

	sstack_p = -1;
}

int
read_state()
{
	if ((sstack_p < 0) || (sstack_p >= SSTACK_SIZE)) {
		error(catgets(nl_fd,NL_SETN,2,"Error: state stack corrupted"));
		emergency_exit();
	}
	return(state_stack[sstack_p]);
}

void
push_state(state)
int	state;
{
	if (sstack_p >= SSTACK_SIZE) {
		error(catgets(nl_fd,NL_SETN,2,"Error: state stack corrupted"));
		emergency_exit();
	}
	state_stack[++sstack_p] = state;
}

void
pop_state()
{
	if (sstack_p-- < 0) {
		error(catgets(nl_fd,NL_SETN,2,"Error: state stack corrupted"));
		emergency_exit();
	}
}
#endif
