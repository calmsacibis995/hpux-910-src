/**			initialize.c			**/

/*
 *  @(#) $Revision: 72.3 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 * Initialize - read in all the defaults etc etc 
 */


#include <termio.h>
#include <pwd.h>
#include <time.h>
#include <signal.h>

#include "headers.h"

#ifdef NLS
# define NL_SETN   22
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS

char 		loginname[SLEN];	/* buffer for expand_logname()	  */

extern char *HPUX_ID;

int 
initialize( initscreen_too )

	int             initscreen_too;

{
	/*
	 *  initialize the whole ball of wax.   If "initscreen_too" then
	 *  call init_screen where appropriate..
	 */


	struct passwd  *pass;

	register int    i, j;
	void            quit_signal(), term_signal(), ill_signal(),
	                fpe_signal(), bus_signal(), segv_signal(),
	                alarm_signal(), pipe_signal(), hup_signal();
#ifdef SIGWINCH
	void		wind_signal();
#endif

#ifdef SIGTSTP
	void             sig_user_stop(), sig_return_from_user_stop();
#endif

	char            buffer[SLEN], *cp;


	userid = getuid();
	groupid = getgid();
	egroupid = getegid();

	strcpy( home, ((cp = getenv("HOME")) == NULL) ? "" : cp );
	strcpy( shell, ((cp = getenv("SHELL")) == NULL) ? default_shell : cp );
	strcpy( pager, ((cp = getenv("PAGER")) == NULL) ? default_pager : cp );
	strcpy( tmpdir, ((cp = getenv("TMPDIR")) == NULL) ? default_tmpdir : cp );

	for (i=0; i<5; i++)
		HPUX_ID++;
	strcpy( VERSION, HPUX_ID );
	for ( i = 0, j = 0; i < strlen(VERSION); i++ )	/* set version_id  */
		if ( *(VERSION + i) != '$' )
			revision_id[j++] = (char)tolower((int)*(VERSION + i));

	revision_id[j] = '\0';

	for ( i = strlen(revision_id)-1; i > 0 && revision_id[i] == ' '; i-- );

	revision_id[++i] = '\0';


#ifdef NLS
	/*
	 * Elm can speak native language.... now set ready 
	 */

	strcpy( lang, ((cp = getenv("LANG")) == NULL) ? "" : cp );
	nl_init( lang );
	nl_fd = catopen( "elm", 0 );
#endif NLS


#ifdef DEBUG
	if ( debug ) {				/* setup for dprint() statements! */

		char            newfname[SLEN], filename[SLEN];

		sprintf( filename, "%s/%s", home, DEBUGFILE );

		if ( access(filename, ACCESS_EXISTS) == 0 ) {	/* already one! */
			sprintf( newfname, "%s/%s", home, OLDEBUG );
			(void) unlink( newfname );
			(void) link( filename, newfname );
			(void) unlink( filename );
		}

		/*
		 * Note what we just did up there: we always save the old
		 * version of the debug file as OLDEBUG, so users can mail
		 * copies of bug files without trashing 'em by starting up
		 * the mailer.  Dumb, subtle, but easy enough to do! 
		 */

		if ( (debugfile = fopen(filename, "w")) == NULL ) {
			debug = 0;		/* otherwise 'leave' will try to log! */
			fprintf(stderr, catgets(nl_fd,NL_SETN,1,"Could not open file %s for debug output!\n"), filename);
			hp_terminal = FALSE;
			leave( 1 );
		}

		chown( filename, userid, groupid );	/* file owned by user */

		fprintf( debugfile,
			 "Debug output of the ELM program (at debug level %d).  %s\n\n",
			 debug, VERSION);
	}
#endif


	if ( initscreen_too )			/* don't set up unless we need to! */
		if ( (i = InitScreen()) < 0 ) {
			if ( i == -1 ) {
				printf(catgets(nl_fd,NL_SETN,2,"Sorry, but you must specify what type of terminal you're on if you want to\n"));
				printf(catgets(nl_fd,NL_SETN,3,"run the \"elm\" program...(you need your environment variable \"TERM\" set)\n"));
				dprint(1, (debugfile, "No $TERM variable in environment!\n"));

			} else if ( i == -2 ) {
				printf(catgets(nl_fd,NL_SETN,4,"You need a cursor-addressable terminal to run \"elm\" and I can't find any\n"));
				printf(catgets(nl_fd,NL_SETN,5,"kind of termcap entry for \"%s\" - check your \"TERM\" setting...\n"),
				       getenv("TERM"));
				dprint(1,
				       (debugfile, "$TERM variable is an unknown terminal type!\n"));

			} else {
				printf(catgets(nl_fd,NL_SETN,6,"Failed trying to initialize your terminal entry: unknown return code %d\n"), i);
				dprint(1, (debugfile, "Initscreen returned unknown code: %d\n",
					   i));
			}

			exit( 1 );
		}

	if ( debug < 5 ) {		/* otherwise let the system trap    */
		signal( SIGINT, SIG_IGN );
		signal( SIGQUIT, quit_signal );	/* Quit signal 	            */
		signal( SIGTERM, term_signal );	/* Terminate signal         */
		signal( SIGILL, ill_signal );	/* Illegal instruction      */
		signal( SIGFPE, fpe_signal );	/* Floating point exception */
		signal( SIGBUS, bus_signal );	/* Bus error  		    */
		signal( SIGSEGV, segv_signal );	/* Segmentation Violation   */
		signal( SIGHUP, hup_signal );	/* HangUp (line dropped)    */

	} else {
		dprint(3, (debugfile,
			   "\n*** Elm-Internal Signal Handlers Disabled due to debug level %d ***\n\n",
			   debug));
	}

#ifdef SIGWINCH
	/* set up handler and state stack for window resizing */
	signal(SIGWINCH, wind_signal);
	init_sstack();
	push_state(WA_NOT);
#endif

	signal( SIGALRM, alarm_signal );	/* Process Timer Alarm	    */
	signal( SIGPIPE, pipe_signal );		/* Illegal Pipe Operation   */


#ifdef SIGTSTP
	signal( SIGTSTP, sig_user_stop );	/* Suspend signal from tty  */
	signal( SIGCONT, sig_return_from_user_stop );	/* Continue Process */
#endif


	get_term_chars();

	gethostname( hostname, sizeof(hostname) );


#ifdef PREFER_UUCP
	strcpy( BOGUS_INTERNET, "@" );
	strcat( BOGUS_INTERNET, hostname );
#endif


	if ( (cp = getenv("LOGNAME")) == NULL )

		if ( (cp = getlogin()) == NULL )
			cuserid( username );
		else
			strcpy( username, cp );

	else
		strcpy( username, cp );

	/*
	 * now let's get the full username.. 
	 */

	if ( (pass = getpwnam(username)) == NULL ) {
		error( catgets(nl_fd,NL_SETN,7,"Couldn't read password entry??") );
		strcpy( full_username, username );

	} else {
		for ( i = 0, j = 0; pass->pw_gecos[i] != '\0' && pass->pw_gecos[i] != ',';
		     i++ )
			if ( pass->pw_gecos[i] == '&' ) {
				dflt_full_name[j] = '\0';
				strcat( dflt_full_name, expand_logname() );
				j = strlen( dflt_full_name );
			} else
				dflt_full_name[j++] = pass->pw_gecos[i];

		dflt_full_name[j] = '\0';
		strcpy( full_username, dflt_full_name );
	}

	if ( (cp = getenv("EDITOR")) == NULL )
		strcpy( editor, default_editor );
	else
		strcpy( editor, cp );

	strcpy( alternative_editor, editor );	/* this one can't be changed! */

	if ( (cp = getenv("VISUAL")) == NULL )
		strcpy( visual_editor, default_editor );
	else
		strcpy( visual_editor, cp );

	if ( (cp = getenv("NAMESERVER")) == NULL )
		nameserver[0] = '\0';		/* none! */
	else
		strcpy( nameserver, cp );

	if ( !mail_only ) {
		mailbox[0] = '\0';
		strcpy( prefixchars, "> " );		/* default message prefix */
		strcpy( printout, "pr %s | lp" ); 	/* default print command  */
		sprintf( calendar_file, "%s/%s", home, dflt_calendar_file );
	}

	local_signature[0] = remote_signature[0] = '\0';	/* NULL! */
	sprintf( mailbox, "%s/%s", home, "mbox" );
	strcpy( savefile, mailbox );
	sprintf( folders, "%s/%s", home, "Mail" );

	read_rc_file();				/* reading the .elm/elmrc next... */

	if ( hp_terminal > 1 )
		hp_terminal = FALSE;

	if ( hp_softkeys > 1 )
		hp_softkeys = FALSE;

	if ( mini_menu > 1 )
		mini_menu = FALSE;

	/*
	 *  now try to expand the specified filename... 
	 */

	if ( strlen(infile) > 0 ) {
		(void) expand_filename( infile );

		if ( user_access(infile, READ_ACCESS) != 0 ) {

			int             err = errno;

			dprint( 1, (debugfile,
				   "Error: given file %s as mailbox - unreadable (%s)!\n",
				   infile, error_name(err)) );
			fprintf( stderr, catgets(nl_fd,NL_SETN,8,"Can't open mailbox '%s':%s\n"), infile,
				 error_description(err) );
			exit( 1 );
		}
	}

	/*
	 *  check to see if the user has defined a LINES or COLUMNS
	 *  value different to that in the termcap entry (for
	 *  windowing systems, of course!) 
	 */

	ScreenSize( &LINES, &COLUMNS );

	if ( (cp = getenv("LINES")) != NULL && isdigit(*cp) ) {
		sscanf( cp, "%d", &LINES );
		LINES -= 1;		/* kludge for HP Window system? ... 
				 	 * LINES is used for cursor position index */
	}

	if ( (cp = getenv("COLUMNS")) != NULL && isdigit(*cp) )
		sscanf( cp, "%d", &COLUMNS );	/* COLUMNS is used for the 
						 * number of char per line */

	/*
	 *  fix the shell if needed 
	 */

	if ( shell[0] != '/' ) {
		sprintf( buffer, "/bin/%s", shell );
		strcpy( shell, buffer );
	}

	if ( !mail_only ) {

		/*
		 * if not have 2622 ESC sequence, not use softkeys 
		 */

		if (! hp_device){
			hp_terminal = FALSE;
			hp_softkeys = FALSE;
		}

		mailbox_defined = (mailbox[0] != '\0');

		/*
		 * get the cursor control keys... 
		 */

		cursor_control = FALSE;

		if (! hp_terminal)
			hp_softkeys = FALSE;

		if ( return_value_of("ks") != NULL && return_value_of("ke") != NULL ){
			has_transmit = TRUE;
			transmit_functions( ON );		/* disable local echo */
		}

		if ( hp_terminal && (cp = return_value_of("ku")) != NULL )
			if ( strlen(cp) == 2 ) {
				strcpy( up, cp );
				if ( (cp = return_value_of("kd")) == NULL )
					cursor_control = FALSE;
				else if ( strlen(cp) != 2 )
					cursor_control = FALSE;
				else {
					strcpy( down, cp );
					cursor_control = TRUE;
				}
			}

		strcpy( start_highlight, "->" );
		end_highlight[0] = '\0';

		if ( !arrow_cursor ) {		/* try to use inverse bar instead */

			cp = return_value_of( "so" );

			if ( strlen(cp) != 0 ) {

				strcpy( start_highlight, cp );
				cp = return_value_of( "se" );

				if ( strlen(cp) == 0 ) {
					strcpy( start_highlight, "->" );
				} else {
					strcpy( end_highlight, cp );
					has_highlighting = TRUE;
				}
			} 
		}
	}

	/*
	 *  allocate the first KLICK headers...
	 */

       if ( (header_table = (struct header_rec *) calloc(KLICK,
                                     sizeof(struct header_rec))) == NULL ) {
               fprintf( stderr, catgets(nl_fd,NL_SETN,9,
                        "\n\r\n\rCouldn't allocate initial headers!\n\r\n") );
               dprint (1, (debugfile,
                       "ERROR - calloc(): could not calloc (%d,%d) for %s\n",
                       KLICK, sizeof(struct header_rec),"headers"));
               hp_terminal = FALSE;
               leave( 1 );
       }



	max_headers = KLICK;		/* we have those preallocated */

	if ( !mail_only ) {
		if ( mini_menu )
			headers_per_page = LINES - 13;
		else
			headers_per_page = LINES - 8;		/* 5 more headers! */
		if ( headers_per_page == 0 || headers_per_page < 0 ) {
			fprintf( stderr, catgets(nl_fd,NL_SETN,10,"\n\r\n\rCouldn't show the header index!\n\r") );
			fprintf( stderr, catgets(nl_fd,NL_SETN,11,"\n\rPlease increase the column of window!\n\r") );
			hp_terminal = FALSE;
			leave( 1 );
		}

		newmbox( 1, FALSE, TRUE );			/* read in the mailbox! */
	}


#ifdef DEBUG
	if ( debug >= 2 && debug < 10 ) {
		fprintf( debugfile,
		 	 "hostname = %-20s \tusername = %-20s \tfullname = %-20s\n",
			 hostname, username, full_username );

		fprintf( debugfile,
		 	 "home     = %-20s \teditor   = %-20s \tmailbox  = %-20s\n",
			 home, editor, mailbox );

		fprintf( debugfile,
		 	 "infile   = %-20s \tfolders  = %-20s \tprintout = %-20s\n",
			 infile, folders, printout );

		fprintf( debugfile,
			 "savefile = %-20s \tprefix   = %-20s \tshell    = %-20s\n\n",
			 savefile, prefixchars, shell );

		if ( signature )
			fprintf( debugfile,
				 "local-signature = \"%s\" and \tremote-signature = \"%s\"\n\n",
				 local_signature, remote_signature );
	}
#endif


}


int 
get_term_chars()

{
	/*
	 *  This routine sucks out the special terminal characters
	 *  ERASE and KILL for use in the input routine.  
	 */


	struct termio   term_buffer;


	if ( ioctl(STANDARD_INPUT, TCGETA, &term_buffer) == -1 ) {
		dprint( 1, (debugfile,
		   	"Error: %s encountered on ioctl call (get_term_chars)\n",
			error_name(errno)) );

		/*
		 * set to defaults for terminal driver 
		 */

		backspace = BACKSPACE;
		kill_line = ctrl('U');
		eof_char  = ctrl('D');

	} else {
		backspace = term_buffer.c_cc[VERASE];
		kill_line = term_buffer.c_cc[VKILL];
		eof_char  = term_buffer.c_cc[VEOF];
	}
}


char 
*expand_logname()

{
	/*
	 *  Return logname in a nice format (for expanding "&" in the
	 *  /etc/passwd file) 
	 */


	register int    i;


	if ( strlen(username) == 0 )
		loginname[0] = '\0';
	else {
		loginname[0] = toupper( username[0] );

		for ( i = 1; username[i] != '\0'; i++ )
			loginname[i] = (char)tolower( (int)username[i] );

		loginname[i] = '\0';
	}

	return ( (char *) loginname );
}
