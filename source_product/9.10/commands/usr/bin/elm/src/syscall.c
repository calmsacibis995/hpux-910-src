/**			syscall.c			**/

/*
 *  @(#) $Revision: 72.5 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  These routines are used for user-level system calls, including the
 *  '!' command and the '|' commands...
 *
 */

#include <signal.h>
#include <termio.h>
#include <sys/wait.h>

#include "headers.h"


#ifdef NLS
# define NL_SETN   44
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS

int		*get_version();

extern int	num_char;

int
subshell()
{
	/*
	 *  spawn a subshell with either the specified command
	 *  returns non-zero if screen rewrite needed
	 */


	char            command[SLEN];
	int             ret;


	PutLine0( LINES - 3, COLUMNS - 40, catgets(nl_fd,NL_SETN,1,"(use the shell name for a shell)") );
	PutLine0( LINES - 2, 0, catgets(nl_fd,NL_SETN,2,"Shell Command: ") );
	num_char = strlen(catgets(nl_fd,NL_SETN,2,"Shell Command: ") );

	if ( hp_terminal )
		define_softkeys( CANCEL );

	command[0] = '\0';

	if ( optionally_enter( command, LINES - 2, num_char, FALSE ) ) {

		if ( hp_terminal )
			define_softkeys( MAIN );

		MoveCursor( LINES - 2, 0 );
		CleartoEOLN();
		return ( 0 );
	} else if ( strlen(command) == 0 ) {

		if ( hp_terminal )
			define_softkeys( MAIN );

		MoveCursor( LINES - 2, 0 );
		CleartoEOLN();
		return ( 0 );
	}

	MoveCursor( LINES, 0 );
	CleartoEOLN();
	Raw( OFF );

	if ( hp_terminal )
		softkeys_off();

	if ( has_transmit )
		transmit_functions( OFF );

	ret = system_call( command, USER_SHELL, (char **)0 );

	if ( user_level == 0 )
		PutLine0(LINES, 0, catgets(nl_fd,NL_SETN,3,"\n\n(If the screen is messed up, please use ^L to redraw it)\n"));

	PutLine0( LINES, 0, catgets(nl_fd,NL_SETN,4,"\n\nPress any key to return to ELM: ") );

	Raw( ON );
#ifdef SIGWINCH
	clearerr(stdin);
	while(getchar() == EOF) {
		if (feof(stdin))
			break;
		clearerr(stdin);
	}
#else
	(void)getchar();
#endif

	if ( has_transmit )
		transmit_functions( ON );

	if ( ret != 0 ) {
		char tmp[SLEN];

		(void) sprintf(tmp, catgets(nl_fd,NL_SETN,5,"Return code was %d"), ret );
		set_error(tmp);
	}

	return ( 1 );

}


int
system_call( string, shell_type, argv )

	int		shell_type;
	char            *string;
	char            **argv;

{
	/*
	 *  execute 'string', setting uid to userid... 
	 *  if shell-type is "SH" /bin/sh is used regardless of the 
	 *  users shell setting.  Otherwise, "USER_SHELL" is sent 
	 *   Some function needs the return status of the executed command.
	 *  In this case, the command is executed as 'exec' command, not as
	 *  standard input of a 'shell'. 'argv' is not used with SH or USER_SH.
	 */


	int             stat = 0, 
			pid, 
			w,
	                status;
#ifdef SIGWINCH
	struct winsize	w_before, w_after;
#endif
	void		(*istat) (), 
#ifdef SIGWINCH
			(*wstat) (),
#endif
			(*qstat) ()

#ifdef SIGSTOP
			,(*tstat) ()
#endif
			;


	dprint( 2, (debugfile,
	    "System Call: %s\n\t%s\n", shell_type == SH ? "/bin/sh" 
				 	: (shell_type == EX_CMD ? argv[0] : shell),
					  string) );

#ifdef SIGWINCH
	ioctl(0, TIOCGWINSZ, &w_before);
#endif

#ifdef SIGSTOP
	tstat = signal( SIGTSTP, SIG_IGN );
#endif

#ifdef NO_VM						/*machine without virtual memory! */
	if ( (pid = fork()) == 0 )
#else
	if ( (pid = vfork()) == 0 )
#endif

	{
		setgid( groupid );			/* and group id	     	   */
		setuid( userid );			/* back to the normal user!       */
		/*
		 * There used to be a call to setsid here that was removed.
		 * The call was added to resolve defect DSDe410316 where closing the
		 * elm window too soon resulted in e-mail not being sent (after elm
		 * claimed that the mail was indeed sent!).  The original
		 * solution solved the problem, but setsid removes the controlling
		 * terminal so the pipe command to a pager started to fail.  The
		 * defect repair now resides in the mailmsg2.c file.  The original
		 * implementation fires off the mailer in the background allowing
		 * control to return immediately to elm.  Unfortunately, it is possible
		 * for control to return before the mail is really sent.  The mail
		 * now sends the mail in forground so that control doesn't return
		 * until the mail is really sent.
		 */

		(void) signal( SIGHUP, SIG_DFL); 	/* allow children to handle this one themselves */

		if ( strlen(shell) > 0 && shell_type == USER_SHELL ) {
			execl( shell, get_last(shell), "-c", string, 0 );

		} else if ( shell_type == SH )
			execl( "/bin/sh", "sh", "-c", string, 0 );

		else 
			execvp( string, argv );
		_exit( 127 );
	}

	istat = signal( SIGINT, SIG_IGN );
	qstat = signal( SIGQUIT, SIG_IGN );
#ifdef SIGWINCH
	wstat = signal( SIGWINCH, SIG_IGN );
#endif

	while ( (w = wait(&status)) != pid && w != -1 );

	if ( w == -1 || (status >> 8) != 0 )
		stat = status >> 8;

#ifdef SIGWINCH
	ioctl(0, TIOCGWINSZ, &w_after);
	if ((w_before.ws_row != w_after.ws_row) ||
			(w_before.ws_col != w_after.ws_col))
		setsize(w_after.ws_row, w_after.ws_col);
#endif

#ifdef SIGSTOP
	signal( SIGTSTP, tstat );
#endif

	signal( SIGINT, istat );
	signal( SIGQUIT, qstat );
#ifdef SIGWINCH
	signal( SIGWINCH, wstat );
#endif

	return ( stat );
}


int
do_pipe()

{
	/*
	 * pipe the tagged messages to the specified sequence.. 
	 */


	char            command[SLEN], 
			buffer[VERY_LONG_STRING], 
			message_list[VERY_LONG_STRING - 100];
	register int    ret, 
			tagged = 0, 
			i;


	message_list[0] = '\0';			/* NULL string to start... */

	for ( i = 0; i < message_count; i++ )
		if ( ison(header_table[i].status, TAGGED) ) {

			sprintf( buffer, "%d", header_table[i].index_number );

			if ( strlen(message_list)+strlen(buffer)
					> VERY_LONG_STRING - 100 )
				break;

			sprintf( message_list, "%s %s", message_list,
				 buffer );
			tagged++;
		}

	if ( tagged > 1 ) {
		PutLine0( LINES - 2, 0, catgets(nl_fd,NL_SETN,6,"Pipe tagged msgs to: ") );
		num_char = strlen(catgets(nl_fd,NL_SETN,6,"Pipe tagged msgs to: ") );
		}
	else if ( tagged ) {
		PutLine0( LINES - 2, 0, catgets(nl_fd,NL_SETN,7,"Pipe tagged msg to : ") );
		num_char = strlen(catgets(nl_fd,NL_SETN,6,"Pipe tagged msg to: ") );
		}
	else {
		PutLine0( LINES - 2, 0, catgets(nl_fd,NL_SETN,8,"Pipe current msg to: ") );
		num_char = strlen(catgets(nl_fd,NL_SETN,8,"Pipe current msg to: ") );
		sprintf( message_list, "%d", header_table[current - 1].index_number );
	}

	command[0] = '\0';

	if ( hp_terminal )
		define_softkeys( CANCEL );

	if ( optionally_enter( command, LINES - 2, num_char, FALSE )) {

		if ( hp_terminal )
			define_softkeys( MAIN );

		MoveCursor( LINES - 2, 0 );
		CleartoEOLN();
		return ( 0 );

	} else if ( strlen(command) == 0 ) {

		if ( hp_terminal )
			define_softkeys( MAIN );

		MoveCursor( LINES - 2, 0 );
		CleartoEOLN();
		return ( 0 );
	}

	MoveCursor( LINES, 0 );
	CleartoEOLN();

	if ( strlen(readmail) + strlen(infile) + strlen(message_list)
			+ strlen(command) + 11 > VERY_LONG_STRING ) {
		error( catgets(nl_fd,NL_SETN,18,"Too many messages or too long piped command") );
		return( 1 );
	}

	sprintf( buffer, "%s -m %d -f %s -h %s | %s",
		 readmail, tagged, infile, message_list, command );

	Raw( OFF );

	if ( hp_terminal )
		softkeys_off();

	if ( has_transmit )
		transmit_functions( OFF );

	ret = system_call( buffer, USER_SHELL, (char **)0 );

	PutLine0( LINES, 0, catgets(nl_fd,NL_SETN,9,"\n\nPress any key to return to ELM: ") );
	Raw( ON );

#ifdef SIGWINCH
	clearerr(stdin);
	while(getchar() == EOF) {
		if (feof(stdin))
			break;
		clearerr(stdin);
	}
#else
	(void)getchar();
#endif

	if ( has_transmit )
		transmit_functions( ON );

	if ( ret != 0 ) {
		char tmp[SLEN];

		(void) sprintf(tmp, catgets(nl_fd,NL_SETN,5,"Return code was %d"), ret );
		set_error(tmp);
	}
	return ( 1 );

}


int
print_msg()

{
	/*
	 *  Print current message or tagged messages using 'printout' 
	 *  variable.  Error message iff printout not defined! 
	 */


	char            buffer[VERY_LONG_STRING], 
			filename[SLEN], 
			printbuffer[VERY_LONG_STRING];
	char            message_list[VERY_LONG_STRING - 100];
	register int    figure = 1,
			quotient,
			retcode, 
			tagged = 0, 
			i,
			ver_pref,
			ver_suff,
			*ver_ptr;


	if ( strlen(printout) == 0 ) {
		error( catgets(nl_fd,NL_SETN,11,"Don't know how to print - option \"printmail\" undefined!") );
		return;
	}

	message_list[0] = '\0';				/* reset to null... */

	for ( i = 0; i < message_count; i++ )
		if ( header_table[i].status & TAGGED ) {

			quotient = header_table[i].index_number;
			figure = 1;

			while ( quotient/10 >= 1 ){
				quotient = quotient/10;
				figure++;
			}

			if ( strlen(message_list)+figure+1 >= VERY_LONG_STRING - 100 ){
				error1( catgets(nl_fd,NL_SETN,16,"Too many messages to print - queued up only %d message(s)"), tagged );
				(void) sleep(2);
				break;
			}

			sprintf( message_list, "%s %d", message_list,
				header_table[i].index_number );
			tagged++;
		}

	if ( !tagged )
		sprintf( message_list, " %d", header_table[current - 1].index_number );

	sprintf( filename, "%s/%s%d", tmpdir, temp_print, getpid() );

	if ( in_string(printout, "%s") )
		sprintf( printbuffer, printout, filename );
	else
		sprintf( printbuffer, "%s %s", printout, filename );

	if ( strlen(readmail)+strlen(infile)+strlen(message_list)
	    +strlen(filename)+strlen(printbuffer)+31 > VERY_LONG_STRING ) {
		error(catgets(nl_fd,NL_SETN,17,"Too many messages to print or long printout command"));
		return;
	}

	ver_ptr = get_version( readmail );
	ver_pref = *ver_ptr++;
	ver_suff = *ver_ptr;

	if ( ver_pref > 64 || ( ver_pref == 64 && ver_suff > 3 ) )
		sprintf( buffer, "(%s -w -m %d -p -f %s %s > %s; %s 2>&1) > /dev/null",
		 	readmail, tagged, infile, message_list, filename, printbuffer );
	else
		sprintf( buffer, "(%s -p -f %s %s > %s; %s 2>&1) > /dev/null",
		 	readmail, infile, message_list, filename, printbuffer );

	dprint( 2, (debugfile, "Printing system call...\n") );

	Centerline( LINES, catgets(nl_fd,NL_SETN,12,"queueing...") );

	if ( (retcode = system_call(buffer, SH, (char **)0)) == 0 ) {
		sprintf( buffer, catgets(nl_fd,NL_SETN,13,"Message(s) queued up to print") );
		Centerline( LINES, buffer );
	} else
		error1( catgets(nl_fd,NL_SETN,14,"Printout failed with return code %d"), retcode );

	unlink( filename );				/* remove da temp file! */

}


int
list_folders()

{
	/*
	 *  list the folders in the users FOLDERHOME directory.  This is
	 *  simply a call to "lsf -C"
	 */


	char            buffer[VERY_LONG_STRING];


	CleartoEOS();		/* don't leave any junk on the bottom of the
				 * screen */

	sprintf( buffer, "cd %s;lsf -C", folders );
	printf( catgets(nl_fd,NL_SETN,15,"\n\rContents of your folder directory:\n\r\n\r") );
	system_call( buffer, SH, (char **)0 );
	printf( "\n\r" );

}


int
*get_version( prog )

char *prog;

{
	FILE *fp, *fopen();
	int c, ver[2];
	char line[SHORT_SLEN], string[NLEN], point;
	register char * tp;

	ver[0] = ver[1] = 0;

	if ( (fp = fopen(prog, "r") ) == NULL ) {
		return( (int *)ver );
	} else {
		while( (c=getc(fp)) != EOF) {
			if ( (char)c == '$' ) {
				tp = line;
				while( (c=getc(fp)) != '$' ) {
					*tp++ = c;
					if ( c==EOF || c=='\n' || tp>=line+SHORT_SLEN-2 )
						break;
				}
				*tp = '\0';
			}
			if ( first_word(line, "Revision") ) {
				sscanf( &line[0], "%s%d%c%d", &string[0]
				       ,&ver[0], &point, &ver[1]);
				break;
			}
		}
	}
        return( (int *)ver );
}
