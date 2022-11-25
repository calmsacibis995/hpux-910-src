/**			file_utils.c			**/

/*
 *  @(#) $Revision: 72.4 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  File oriented utility routines for ELM 
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <termio.h>

#include "headers.h"


#ifdef NLS
# define NL_SETN  16
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


long            fsize();

long
bytes( name )

	char           *name;

{
	/*
	 *  return the number of bytes in the specified file.  This
	 *  is to check to see if new mail has arrived....  (also
	 *  see "fsize()" to see how we can get the same information
	 *  on an opened file descriptor slightly more quickly)
	 */


	int             ok = 1;
	struct stat     buffer;


	if ( stat(name, &buffer) != 0 )
		if ( errno != 2 ) {
			dprint( 1, (debugfile,
			        "Error: errno %s on fstat of file %s (bytes)\n",
				error_name(errno), name) );
			Write_to_screen( catgets(nl_fd,NL_SETN,1,"\n\rError attempting fstat on file %s!\n\r"),
				1, name, NULL, NULL );
			Write_to_screen( "** %s - %s **\n\r", 2,
				error_name(errno), error_description(errno), NULL );
			emergency_exit();
		} else
			ok = 0;

	return ( ok ? (long) buffer.st_size : 0L );
}


int
user_access( file, mode )

	char           *file;
	int             mode;

{
	/*
	 *  returns ZERO iff user can access file or "errno" otherwise 
	 */


	int             the_stat = 0, 
			pid, 
			w;
	int             status;
#ifdef SIGWINCH
	struct winsize	w_before, w_after;
#endif
	void		(*istat) (), 
#ifdef SIGWINCH
			(*wstat) (),
#endif
			(*qstat) ();


#ifdef SIGWINCH
	ioctl(0, TIOCGWINSZ, &w_before);
#endif

#ifdef NO_VM					/* machine without virtual memory!! */
	if ((pid = fork()) == 0) {
#else
	if ((pid = vfork()) == 0) {
#endif


		setgid( groupid );
		setuid( userid );		/** back to normal userid **/

		errno = 0;

		if ( access(file, mode) == 0 )
			_exit( 0 );
		else
			_exit( errno != 0 ? errno : 1 );	/* never return zero! */
		_exit( 127 );
	}

	istat = signal( SIGINT, SIG_IGN );
	qstat = signal( SIGQUIT, SIG_IGN );
#ifdef SIGWINCH
	wstat = signal( SIGWINCH, SIG_IGN );
#endif

	while ( (w = wait(&status)) != pid && w != -1 );

	the_stat = status;

#ifdef SIGWINCH
	ioctl(0, TIOCGWINSZ, &w_after);
	if ((w_before.ws_row != w_after.ws_row) ||
			(w_before.ws_col != w_after.ws_col))
		setsize(w_after.ws_row, w_after.ws_col);
	signal( SIGWINCH, wstat );
#endif
	signal( SIGINT, istat );
	signal( SIGQUIT, qstat );

	return ( the_stat );
}


FILE *
user_fopen( file, mode )

	char            *file, 
			*mode;

{
	/*
	 *  Return the results of the 'real' user trying to open the
	 *  specified file.  This is used for cases where the file 
	 *  might already exist -- if, for example, it's owned by
	 *  bin, elm may open it but the user shouldn't be allowed to.
	 */


	int		err = 0; 
	int             pid, 
			w;
	int             status = 0;
#ifdef SIGWINCH
	struct winsize	w_before, w_after;
#endif
	void		(*istat) (), 
#ifdef SIGWINCH
			(*wstat) (),
#endif
			(*qstat) ();


#ifdef SIGWINCH
	ioctl(0, TIOCGWINSZ, &w_before);
#endif

#ifdef NO_VM					/* machine without virtual memory!! */
	if ((pid = fork()) == 0) {
#else
	if ((pid = vfork()) == 0) {
#endif


		FILE           *fp;


		setgid( groupid );
		setuid( userid );		/** back to normal userid **/
		errno = 0;

		if ( (fp = fopen(file, mode)) == NULL ) {
		        dprint( 1, (debugfile,
				    "user_fopen() FAILED -- fopen(\"%s\", \"%s\") set errno=%d\n",
				    file, mode, errno) );
			_exit( errno );
		}
		else {
			fclose( fp );		/* don't just LEAVE it! */
			_exit( 0 );
		}
		_exit( 127 );
	}

	istat = signal( SIGINT, SIG_IGN );
	qstat = signal( SIGQUIT, SIG_IGN );
#ifdef SIGWINCH
	wstat = signal( SIGWINCH, SIG_IGN);
#endif

	while ( (w = wait(&status)) != pid && w != -1 );

#ifdef SIGWINCH
	ioctl(0, TIOCGWINSZ, &w_after);
	if ((w_before.ws_row != w_after.ws_row) ||
			(w_before.ws_col != w_after.ws_col))
		setsize(w_after.ws_row, w_after.ws_col);

	signal(SIGWINCH, wstat);
#endif
	signal(SIGINT, istat);
	signal(SIGQUIT, qstat);

	err = status >> 8;

	/*
	 *  now go ahead and open the file if the child said it was
	 *  okay, and leave any error code in 'errno' for the caller
	 *  to figure out.  (Code fragment by Chip Salzenberg)
	 */

	if ( err ) {
		errno = err;
		return ( (FILE *) NULL );
	} else {
		/*
		 * This make it possible to read the user's file which 
		 * permission is '---rw----' and only the groupid
		 * is allowed.  Use 'user_fclose()' with user_fopen()
		 */

		(void)setresgid( -1, groupid, egroupid );

		errno = 0;
		return ( (FILE *) fopen(file, mode) );
	}
}


int
user_fclose( fdes )

	FILE		*fdes;

{
	/* 
	 * This is used with 'user_fopen()'
	 */

	fclose( fdes );

	if ( mbox_specified == 0 )
		(void)setresgid( -1, egroupid, egroupid );
}


int
copy( from, to )

	char            *from, 
			*to;
	
{
	/*
	 *  this routine copies a specified file to the destination
	 *  specified.  Non-zero return code indicates that something
	 *  dreadful happened! 
	 */


	FILE            *from_file,
			*to_file;
	char            buffer[VERY_LONG_STRING];


	if ( (to_file = fopen(to, "w")) == NULL ) {
		dprint( 1, (debugfile, "Error: could not open %s for writing (copy)\n",
			   to) );
		error1( catgets(nl_fd,NL_SETN,3,"could not open file %s"), to );
		return ( 1 );
	}

	if ( (from_file = user_fopen(from, "r")) == NULL ) {
		dprint( 1, (debugfile, "Error: could not open %s for reading (copy)\n",
			   from) );
		error1( catgets(nl_fd,NL_SETN,3,"could not open file %s"), from );
		fclose( to_file );
		return ( 1 );
	}

	while ( fgets(buffer, VERY_LONG_STRING, from_file) != NULL )
		if ( fputs(buffer, to_file) == EOF ){
		        dprint( 1, (debugfile, 
				    "Error: could not append to %s (copy)\n", to) );
			user_fclose( from_file );
			fclose( to_file );
			unlink( to );
			return( 1 );
		}	

	user_fclose( from_file );
	fclose( to_file );

	return ( 0 );
}


int 
movefile( from, to )

	char            *from, 
			*to;

{
	/*
	 *  This routine moves a specified file to the destination
	 *  specified.  It starts by trying to it all by link'ing.
	 */


	FILE            *from_file, 
			*to_file;
	char            buffer[VERY_LONG_STRING];


	if ( access(from, ACCESS_EXISTS) == -1 )		/* doesn't exist? */
		return( 1 );

	/*
	 *  does the dest file exist?? 
	 */

	if ( access(to, ACCESS_EXISTS) != -1 ) {		/* dest DOES exist! */
		printf( catgets(nl_fd,NL_SETN,4,"File %s already exists!  Overwriting...\n"), to );
		(void) unlink( to );
	}

	/*
	 *  first off, let's try to link() it
	 */

	if ( link(from, to) != -1 ) {


#ifdef DONT_JUST_LINK_EM_TOGETHER
		(void) unlink( from );
#endif


		return( 0 );
	}
		
	/*
	 *  nope.  Let's open 'em both up and move the data... 
	 */

	if ( (from_file = user_fopen(from, "r")) == NULL ) {
		dprint( 1, (debugfile, "Error: could not open %s for reading (copy)\n",
			   from) );
		return(1);
	}
	
	if ( (to_file = user_fopen(to, "w")) == NULL ) {
		dprint( 1, (debugfile, "Error: could not open %s for writing (copy)\n",
			   to) );
		user_fclose( from_file );
		return(1);
	}

	while ( fgets(buffer, VERY_LONG_STRING, from_file) != NULL )
		if ( fputs(buffer, to_file) == EOF ){
		        dprint( 1, (debugfile, 
				    "Error: could not append to %s (move)\n", to) );
			user_fclose( from_file );
			user_fclose( to_file );
			unlink( to );
			return( 1 );
		}	

	user_fclose( from_file );
	user_fclose( to_file );


#ifdef DONT_JUST_LINK_EM_TOGETHER
	(void) unlink( from );
#endif

	return(0);
}


int
append( fd, filename )

	FILE           *fd;
	char           *filename;

{
	/*
	 *  This routine appends the specified file to the already
	 *  open file descriptor.. Returns non-zero if fails.  
	 */


	FILE           *my_fd;
	char            buffer[VERY_LONG_STRING];


	if ( (my_fd = user_fopen(filename, "r")) == NULL ) {
		dprint( 1, (debugfile,
			   "Error: could not open %s for reading (append)\n", filename) );
		return ( 1 );
	}

	while ( fgets(buffer, VERY_LONG_STRING, my_fd) != NULL )
		if ( fputs(buffer, fd) == EOF ){
		        dprint( 1, (debugfile,
			           "Error: could not append to %s (append)\n", filename) );
			force_final_newline( fd );
			user_fclose( my_fd );
			return( 1 );
		}	

	user_fclose( my_fd );

	return ( 0 );
}


int 
check_mailfile_size()

{
	/*
	 *  Check to ensure we have mail.  Only used with the '-z'
	 *  starting option. 
	 */


	char            filename[LONG_FILE_NAME];
	struct stat     buffer;


	if ( infile[0] != '\0' )
		strcpy( filename, infile );
	else {
		strcpy( username, getlogin() );

		if ( strlen(username) == 0 )
			cuserid( username );

		sprintf( filename, "%s%s", mailhome, username );
	}

	expand_filename( filename );

	if ( stat(filename, &buffer) == -1 ) {
		printf( catgets(nl_fd,NL_SETN,5," You have no mail to read.\n") );
		leave( 0 );
	} else if ( buffer.st_size < 2 ) {			/* maybe one byte??? */
		printf( catgets(nl_fd,NL_SETN,5,"You have no mail to read.\n") );
		leave( 0 );
	}
}


int 
create_readmsg_file()

{
	/*
	 *  Creates the file ".current" in the users home directory
	 *  for use with the "readmail" program.
	 */


	FILE           *fd;
	char            buffer[LONG_FILE_NAME];


	sprintf( buffer, "%s/%s", home, readmail_file );

	if ( (fd = fopen(buffer, "w")) == NULL ) {
		dprint( 1, (debugfile,
			"Error: couldn't create file %s - error %s (%s)\n",
			buffer, error_name(errno), "create_readmsg_file")) ;
		return;					/* no error to user */
	}

	if ( current )
		fprintf( fd, "%d%s\n", 
			header_table[current - 1].index_number,infile );
	else
		fprintf( fd, "\n" );

	fclose( fd );
}


long 
fsize( fd )

	FILE           *fd;

{
	/*
	 *  return the size of the current file pointed to by the given
	 *  file descriptor - see "bytes()" for the same function with
	 *  filenames instead of open files...
	 */


	struct stat     buffer;


	(void) fstat( fileno(fd), &buffer );

	return ( (long) buffer.st_size );
}


void
force_final_newline( fdes )

	FILE 		*fdes;

{
	/*
 	 *  Try to replace the last byte of the file with a \n.
 	 *  Called when a write has failed, presumably because
 	 *  a file system is full, to prevent the next message
 	 *  written to the file "vanishing" because the "From "
 	 *  is not at the beginning of the line.  No error 
 	 *  checking - if it doesn't work at least we tried 
	 */
 	

 	(void) fseek( fdes,-1,2 );			/* 1 byte before EOF */
	(void) putc( '\n',fdes );
}
