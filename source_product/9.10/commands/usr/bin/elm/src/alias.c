/**			alias.c				**/

/*
 *  @(#) $Revision: 70.9.1.1 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  This file contains alias stuff
 */


#include <signal.h>
#include <termio.h>

#include "headers.h"


#ifdef NLS
# define NL_SETN   2
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


int		cancelled = FALSE;		/* Cancelled an alias command */

extern int	deleting;
extern int	num_char;

#ifndef DONT_TOUCH_ADDRESSES
    extern int      findnode_has_been_initialized;
#endif

int 
read_alias_files()

{
	/*
	 *  read the system and user alias files, if present.
	 *  Set the flags 'systemfiles' and 'userfiles' accordingly.
	 */


	char            fname[SLEN];
	int             hash;


	if ( (hash = open(system_hash_file, O_RDONLY)) == -1 ) {
		dprint( 2, (debugfile,
			   "Warning: Can't read system hash file %s\n",
			   system_hash_file) );
		goto user;
	}

	read( hash, (char *) system_hash_table, sizeof system_hash_table );
	close( hash );

	/*
	 * and data file opened...
	 */

	if ( (system_data = open(system_data_file, O_RDONLY)) == -1 ) {
		dprint( 1, (debugfile,
			   "Warning: Can't read system data file %s\n", system_data_file));
		goto user;
	}

	system_files++;				/* got the system files! */

	/*
	 *  as a last final check... 
	 */

	read_in_aliases = TRUE;		

	check_for_size_marker( 1 );

user:	sprintf( fname, "%s/%s", home, ALIAS_HASH );

	if ( (hash = open(fname, O_RDONLY)) == -1 ) {
		dprint( 2, (debugfile, "Warning: Can't read user hash file %s\n",
			   fname));
		return;
	}

	read( hash, (char *) user_hash_table, sizeof user_hash_table );
	close( hash );

	sprintf( fname, "%s/%s", home, ALIAS_DATA );

	if ( (user_data = open(fname, O_RDONLY)) == -1 ) {
		dprint( 1, (debugfile,
			   "Warning: can't read user data file %s\n", fname));
		return;
	}

	user_files++;				/* got user files too! */

	/*
	 *  and, just before we leave... 
	 */

	read_in_aliases = TRUE;

	check_for_size_marker( 0 );

}


int
add_alias()

{
	/*
	 *  add an alias to the user alias text file.  Return zero 
	 *  if alias not added in actuality 
	 */


	char            name[SLEN], 
			*address, 
			address1[VERY_LONG_STRING],
	                comment[SLEN];
	char		tmpbuf1[VERY_LONG_STRING];


	PutLine0( LINES - 2, 0, catgets(nl_fd,NL_SETN,1,"Enter alias name: ") );
	CleartoEOLN();

	if ( hp_terminal )
		define_softkeys( CANCEL );

	name[0] = '\0';

	num_char = strlen(catgets(nl_fd,NL_SETN,1,"Enter alias name: "));

	if ( optionally_enter( name, LINES-2, num_char, FALSE ) ){
		cancelled = TRUE;
		return (0);		/* cancelled by ctrl-'D' */
	}

	if ( strlen(name) == 0 ) {
		cancelled = TRUE;
		return (0);
	}

	if ( (address = get_alias_address(name, 0, 0)) != NULL ) {
		dprint( 3, (debugfile,
		        "Attempt to add a duplicate alias [%s] in add_alias\n",
		         address));

		if ( address[0] == '!' ) {
			address[0] = ' ';
			error1( catgets(nl_fd,NL_SETN,2,"already a group with that name:%s"), address );
		} else
			error1( catgets(nl_fd,NL_SETN,3,"already an alias for that: %s"), address );

		return ( 0 );
	}

	(void) sprintf(tmpbuf1, catgets(nl_fd,NL_SETN,4,"Full name for %s: "), name);

	PutLine1( LINES - 2, 0, tmpbuf1 );
	CleartoEOLN();
	comment[0] = '\0';

	num_char = strlen(tmpbuf1);

	if ( optionally_enter( comment, LINES-2, (int)num_char, FALSE ) ) {
		cancelled = TRUE;
		return ( 0 );			/* cancelled by ctrl-'D' */
	}

	(void) sprintf(tmpbuf1, catgets(nl_fd,NL_SETN,5,"Enter address for %s: "), name );

	PutLine1( LINES - 2, 0, tmpbuf1);
	CleartoEOLN();
	address1[0] = '\0';

	num_char = strlen(tmpbuf1);

	if ( optionally_enter( address1, LINES-2, (int)num_char, FALSE ) ) {
		cancelled = TRUE;
		return ( 0 );			/* cancelled by ctrl-'D' */
	}

	if ( strlen(address1) == 0 ) {
		error( catgets(nl_fd,NL_SETN,6,"No address specified!") );
		return ( 0 );
	}

	if ( add_to_alias_text(name, comment, address1) )
		return( 0 );

	return ( 1 );

}


int
add_current_alias()

{
	/*
	 *  alias the current message to the specified name and
	 *  add it to the alias text file, for processing as
	 *  the user leaves the program.  Returns non-zero iff
	 *  alias actually added to file 
	 */


	char            name[SLEN], 
			address1[VERY_LONG_STRING], 
			buffer[VERY_LONG_STRING],
			*address,
			comment[SLEN],
			tmpbuf[VERY_LONG_STRING];
	int		i = 0;


	if ( current == 0 ) {
		dprint( 4, (debugfile,
		        "Add current alias called without any current message!\n") );
		error( catgets(nl_fd,NL_SETN,7,"No message to alias to!") );
		return ( 0 );
	}

	PutLine0( LINES - 2, 0, catgets(nl_fd,NL_SETN,8,"Current message address aliased to: ") );
	CleartoEOLN();

	if ( hp_terminal )
		define_softkeys( CANCEL );

	name[0] = '\0';

	num_char = strlen(catgets(nl_fd,NL_SETN,8,"Current message address aliased to: "));

	if ( optionally_enter( name, LINES-2, num_char, FALSE ) ){
		cancelled = TRUE;
		return ( 0 );			/* cancelled by ctrl-'D' */
	}

	if ( (strlen( name )) == 0 ){		/* cancelled...  	 */
		cancelled = TRUE;
		return ( 0 );
	}

	if ( (address = get_alias_address(name, 0, 0)) != NULL ) {
		dprint( 3, (debugfile,
			   "Attempt to add a duplicate alias [%s] in add_current_alias\n",
			   address) );

		if ( address[1] == '!' ) {
			address[0] = ' ';
			error1( catgets(nl_fd,NL_SETN,2,"already a group with that name:%s"), address );
		} else
			error1( catgets(nl_fd,NL_SETN,3,"already an alias for that: %s"), address );

		return ( 0 );
	}

	get_return( buffer, FALSE );		/* grab the return address of this message*/
	break_down_tolist( buffer, &i, address1, comment );
	for ( i = 0; i < strlen(comment); i++ )
		if ( comment[i] == '(' || comment[i] == ')' )
			comment[i] = ' ';
	remove_spaces( comment );

	(void) sprintf(tmpbuf, catgets(nl_fd,NL_SETN,11,"Full name for %s: "), name);

	PutLine1( LINES - 2, 0, tmpbuf );
	num_char = strlen(tmpbuf);
	CleartoEOLN();

	if ( optionally_enter( comment, LINES-2, (int)num_char, TRUE ) ){
		cancelled = TRUE;
		return( 0 );			/* cancelled by ctrl-'D' */
	}

#ifdef OPTIMIZE_RETURN
	optimize_return(address1);
#endif

	if ( strlen(comment) != 0 )
		PutLine3( LINES - 2, 0, "%s (%s) = %s", name, comment, address1 );
	else
		PutLine2( LINES - 2, 0, "%s = %s", name, address1 );

	CleartoEOLN();

	if ( add_to_alias_text(name, comment, address1) )
		return( 0 );

	return ( 1 );

}


int 
add_to_alias_text( name, comment, address )

	char            *name, 
			*comment, 
			*address;

{
	/*
	 *  Add the data to the user alias text file.  Return zero if we
	 *  succeeded, 1 if not
	 */


	FILE            *file;
	char            fname[SLEN];
	int		count;


	sprintf( fname, "%s/%s", home, ALIAS_TEXT );

	if ( (file = user_fopen(fname, "a")) == NULL ) {
		dprint( 2, (debugfile,
		        "Failure attempting to add alias to file %s within %s",
			fname, "add_to_alias_text") );
		dprint( 2, (debugfile, "** %s - %s **\n", error_name(errno),
			error_description(errno)) );
		error1( catgets(nl_fd,NL_SETN,12,"couldn't open %s to add new alias!"), fname );
		return ( 1 );
	}
	
	count = fprintf( file, "%s = %s%s%s\n", name, 
		strlen( comment ) ? comment:"", 
		strlen( comment ) ? " = ":"", address );

	if ( count != strlen(name) + strlen(comment) + strlen(address) 
		    + (strlen(comment) ? 7:4) ) {

	        error1(catgets(nl_fd,NL_SETN,13,"Couldn't add new alias to file %s"), fname);
		fseek( file, (long)(-count), 2 );
		fputs( "\n", file );
		user_fclose( file );
		return( 1 );
	}

	user_fclose( file );

	return ( 0 );
}


int 
show_alias_menu()

{
	/*
	 * This routine simply showout the alias menu.
	 */


	SetCursor( LINES - 8, 0 );
	CleartoEOLN();
	MoveCursor( LINES - 7, 0 );
	CleartoEOLN();
	MoveCursor( LINES - 6, 0 );
	CleartoEOLN();
	MoveCursor( LINES - 5, 0 );
	CleartoEOLN();

	Centerline( LINES - 8, catgets(nl_fd,NL_SETN,14,"Alias commands") );
	Centerline( LINES - 6,
		    catgets(nl_fd,NL_SETN,15,"A)lias current msg,  M)ake new alias,  D)elete alias  or  Check a P)erson.") );
	Centerline( LINES - 5, 
		    catgets(nl_fd,NL_SETN,16,"E)xpand and check alias,  List  U)ser  or  S)ystem aliases.") );

}

#ifdef SIGWINCH
extern int      resized;        /* flag indicating window resize occurred */
#endif

int 
alias()

{
	/*
	 *  work with alias commands... 
	 */


	char            name[SLEN], 
			*address, 
			ch;


#ifdef ENABLE_NAMESERVER
	char            old_resolver[LONG_FILE_NAME];
#endif


	int             newaliases = 0,
			ret = 0;


	if ( mini_menu )
		show_alias_menu();

	/*
	 *  now let's ensure that we've initialized everything! 
	 */


#ifndef DONT_TOUCH_ADDRESSES

	if ( !findnode_has_been_initialized ) {
		if ( !mail_only && warnings )
			error( catgets(nl_fd,NL_SETN,17,"initializing internal tables...") );

#ifndef USE_DBM
		get_connections();
		open_domain_file();
#endif

		init_findnode();
		clear_error();
		findnode_has_been_initialized = TRUE;
	}
#endif


	while ( 1 ) {
#ifdef SIGWINCH
		resized = 0;
#endif
		prompt( catgets(nl_fd,NL_SETN,18,"Alias: ") );

		if ( hp_terminal )
			define_softkeys( ALIAS );

		if ( !cancelled )
			CleartoEOLN();
		else{
			cancelled = FALSE;
			CleartoEOS();
		}

		ch = (char)tolower( ReadCh() );
		CleartoEOS();
		clear_error();

		dprint( 3, (debugfile, "\n-- Alias command: %c\n\n", ch) );

		switch ( ch ) {

		case '?':
			alias_help();
			cancelled = TRUE;
			break;

		case ctrl('L'):
			ClearScreen();
			update_title();
			last_header_page = -1;
			show_headers();
			if ( mini_menu )
				show_alias_menu();
			break;

		case 'a':
			newaliases += add_current_alias();
			if ( newaliases ) {
				install_aliases();
				newaliases = 0;
			}
			ClearLine( LINES - 2 );
			ClearLine( LINES - 1 );
			break;

		case 'd':
			newaliases += delete_alias();
			if ( newaliases ) {
				install_aliases();
				newaliases = 0;
			}
			ClearLine( LINES - 2 );
			ClearLine( LINES - 1 );
			break;

		case 'm':
			newaliases += add_alias();
			if ( newaliases ) {
				install_aliases();
				newaliases = 0;
			}
			ClearLine( LINES - 2 );
			ClearLine( LINES - 1 );
			break;

		case RETURN:
		case LINE_FEED:
		case 'q':
		case 'x':
		case 'r':
			return;

		case 'p':
			PutLine0( LINES - 2, 0, catgets(nl_fd,NL_SETN,21,"Check for person: ") );
			num_char = strlen(catgets(nl_fd,NL_SETN,21,"Check for person: ") );
			CleartoEOLN();

			if ( hp_terminal )
				define_softkeys( CANCEL );

			name[0] = '\0';

			if ( optionally_enter( name, LINES-2, num_char, FALSE ) ){
				cancelled = TRUE;
				break;
			}


#ifdef ENABLE_NAMESERVER
			strcpy( old_resolver, nameserver );
			nameserver[0] = '\0';		/* turn it off for this */
#endif

			if ( strlen(name) == 0 )
				cancelled = TRUE;


			if ((address = get_alias_address(name, 0, 0)) != NULL) {
				ClearScreen();

				if ( address[0] == '!' ) {
					address[0] = ' ';
					PutLine1( 3, 0,
						catgets(nl_fd,NL_SETN,22,"Group alias:\n\r%s"), address );
				} else
					PutLine1( 3, 0,
					   catgets(nl_fd,NL_SETN,23,"Aliased address:\n\r%s"),
					   address );

				PutLine0(LINES - 1, 0,
					catgets(nl_fd,NL_SETN,27,"\n\nPress any key to return to Alias Main Menu: "));

#ifdef SIGWINCH
				clearerr(stdin);
				while(getchar() == EOF) {
					if (feof(stdin))
						break;
					clearerr(stdin);
				}
#else
				(void) getchar();
#endif

				/* We must redraw the screen */

				ClearScreen();
				update_title();
				last_header_page = -1;
				show_headers();

				if ( mini_menu )
					show_alias_menu();
#ifdef SIGWINCH
				resized = 0;
#endif
			} else if ( !cancelled )
				error( catgets(nl_fd,NL_SETN,24,"not found") );


#ifdef ENABLE_NAMESERVER
			strcpy(nameserver, old_resolver);
#endif


			break;

		case 's':
			PutLine0(LINES-2, 0, catgets(nl_fd,NL_SETN,25,"List system aliases:\r\n"));
			CleartoEOS();

			if ( hp_terminal )
				softkeys_off();

			Raw( OFF );

			if ( has_transmit )
				transmit_functions( OFF );
			
			ret = list_alias( TRUE );

			if ( user_level == 0 )
				fprintf( stdout,
				catgets(nl_fd,NL_SETN,26,"\n\n(If the screen is messed up, please use ^L to redraw it)\n"));

			fprintf( stdout, catgets(nl_fd,NL_SETN,27,"\n\nPress any key to return to Alias Main Menu: ") );
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

			/* We must redraw the screen */

			ClearScreen();
			update_title();
			last_header_page = -1;
			show_headers();

			if ( mini_menu )
				show_alias_menu();
#ifdef SIGWINCH
			resized = 0;
#endif

			if ( ret != 0 )
				error1( catgets(nl_fd,NL_SETN,28,"Can't read system %s file"), 
				       ret == 1 ? "hash" : "data" );

			break;

		case 'u':
			PutLine0(LINES-2, 0, catgets(nl_fd,NL_SETN,29,"List user aliases:\r\n"));
			CleartoEOS();

			if ( hp_terminal )
				softkeys_off();

			Raw( OFF );

			if ( has_transmit )
				transmit_functions( OFF );
			
			ret = list_alias( FALSE );

			if ( user_level == 0 )
				fprintf( stdout,
				catgets(nl_fd,NL_SETN,26,"\n\n(If the screen is messed up, please use ^L to redraw it)\n"));

			fprintf( stdout, catgets(nl_fd,NL_SETN,27,"\n\nPress any key to return to Alias Main Menu: ") );
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

			/* We must redraw the screen */

			ClearScreen();
			update_title();
			last_header_page = -1;
			show_headers();

			if ( mini_menu )
				show_alias_menu();

#ifdef SIGWINCH
			/* avoid second screen redraw */
			resized = 0;
#endif

			if ( ret != 0 )
				error1( catgets(nl_fd,NL_SETN,32,"Can't read user %s file"), 
				       ret == 1 ? "hash" : "data" );

			break;

	    	case 'e':
/*			PutLine0( LINES-2,0,"Fully expand alias: " ); */
			PutLine0( LINES - 2, 0, catgets(nl_fd,NL_SETN,61,"Fully expand alias: ") );
			num_char = strlen(catgets(nl_fd,NL_SETN,61,"Fully expand alias: ") );
		        CleartoEOS();

			if ( hp_terminal )
				define_softkeys( CANCEL );

			name[0] = '\0';

			if ( optionally_enter( name, LINES-2, num_char, FALSE ) ){
				cancelled = TRUE;
				break;
			}


#ifdef ENABLE_NAMESERVER
		        strcpy( old_resolver, nameserver );
		        nameserver[0] = '\0';		/* turn it off for this */
#endif


		        if ( (address = get_alias_address(name, 1, 0)) != NULL ) {
	                 	ClearScreen();
/*				PutLine1( 3,0,"Aliased address:\n\r%s", address ); */
                PutLine1( 3,0,catgets(nl_fd,NL_SETN,62,"Aliased address:\n\r%s"),address);
/*	                	PutLine0( LINES-1,0,"Press any key to continue: " ); */
                        PutLine0( LINES-1,0,catgets(nl_fd,NL_SETN,63,"Press any key to continue: "));
#ifdef SIGWINCH
				clearerr(stdin);
				while(getchar() == EOF) {
					if (feof(stdin))
						break;
					clearerr(stdin);
				}
#else
				(void) getchar();
#endif

				/* We must redraw the screen */

				ClearScreen();
				update_title();
				last_header_page = -1;
				show_headers();

				if ( mini_menu )
					show_alias_menu();
#ifdef SIGWINCH
				resized = 0;
#endif

		        } else {
				ClearLine( LINES - 2 );
				ClearLine( LINES - 1 );
				error( catgets(nl_fd,NL_SETN,24,"not found") );
			}

#ifdef ENABLE_NAMESERVER
		        strcpy( nameserver, old_resolver );
#endif

		     	break;


#ifdef SIGWINCH
		 case (char)NO_OP_COMMAND:
			break;
#endif

		 default:
			error( catgets(nl_fd,NL_SETN,38,"Invalid input!") );

		}
#ifdef SIGWINCH
		if (resized) {
			/* screen was resized so we need to redraw it */
			resized = 0;
			ClearScreen();
			update_title();
			last_header_page = -1;
			show_headers();
			if (mini_menu)
				show_alias_menu();
			show_last_error();
		}
#endif

	}
}


int 
install_aliases()

{
	/*
	 *  run the 'elmalias' program and install the newly
	 *  added aliases before going back to the main program! 
	 */


	char		*argv[2],
			last_of_cmd[SLEN],
			temp_cmd[SLEN];
	int		err;
	FILE		*fd;


	error( deleting ? 
	       catgets(nl_fd,NL_SETN,39,"Deleting alias...") : catgets(nl_fd,NL_SETN,40,"Adding new aliases..."));

	/*
	 * Now open the working file.
	 * I need the return value of the shell command 'elmalias'.
	 * If use 'sh -c .....', the return value is always '0'
	 * returned by shell program.
	 */

	sprintf( temp_cmd, "%s/%s%d", tmpdir, temp_alias, getpid() );

	if ( (fd=fopen( temp_cmd, "w" )) == NULL ) {
		err = errno;
		error1( "Failed to execute 'elmalias' command: %s", error_name(err) );
		return;
	}

	err = fprintf( fd, "#!/bin/sh\n%s\nexit $?", newalias );

	fclose( fd );
	chmod( temp_cmd, 00770 );	/* enable to execute */

	strcpy( last_of_cmd, get_last( temp_cmd ) );
	argv[0] = last_of_cmd;
	argv[1] = (char *) 0;

	Raw( OFF );

	if ( has_transmit )
		transmit_functions( OFF );

	if ( system_call(temp_cmd, EX_CMD, argv) == 0) {
		Raw( ON );
		if ( has_transmit )
			transmit_functions( OFF );
		error( catgets(nl_fd,NL_SETN,41,"Re-reading the database in...") );
		read_alias_files();
		error( !deleting ? 
		    catgets(nl_fd,NL_SETN,42,"New aliases installed successfully") : 
		    catgets(nl_fd,NL_SETN,43,"Delete aliases successfully") );
	} else {
		Raw( ON );
		if ( has_transmit )
			transmit_functions( OFF );
		error( catgets(nl_fd,NL_SETN,44,"'elmalias' failed.  Please check 'aliases.text' or 'elmalias'."));
	}

	unlink( temp_cmd );		/* clear the working area */

	if ( deleting )
		deleting = FALSE;

	if ( has_transmit )
		transmit_functions( ON );
}


void
a_help_menu()
{
	MoveCursor( LINES - 3, 0 );
	CleartoEOS();

	if ( !mini_menu )
		lower_prompt( catgets(nl_fd,NL_SETN,45,"Key you want help for : ") );
	else {
		Centerline( LINES - 3,
			    catgets(nl_fd,NL_SETN,46,"Enter key you want help for, '?' for list or '.' to leave help") );
		lower_prompt( catgets(nl_fd,NL_SETN,47,"Key : ") );
	}
}

int 
alias_help()
{
	/*
	 *  help section for the alias menu...
	 */


	char            ch;


#ifdef SIGWINCH
	push_state(WA_AHM);
#endif
	a_help_menu();

	while ( (ch = (char)tolower(ReadCh())) != '.' ) {
		switch ( ch ) {
		case '?':
			if ( hp_terminal )
				softkeys_off();

			display_helpfile( ALIAS_HELP );

			/* We must redraw the screen */

			ClearScreen();
			update_title();
			last_header_page = -1;
			show_headers();

			if ( mini_menu )
				show_alias_menu();

#ifdef SIGWINCH
			/* since we have redrawn screen anyway, clear resized
			   flag to avoid potential double redraw. */
			resized = 0;

			pop_state();
#endif
			return;

		case 'a':
			error(
			      catgets(nl_fd,NL_SETN,48,"a = Add return address of current message to alias database."));
			break;

		case 'd':
			error(
			      catgets(nl_fd,NL_SETN,57,"d = Delete user alias, deleting from alias database when done."));
			break;
		case 'm':
			error(
			      catgets(nl_fd,NL_SETN,49,"m = Make new user alias, adding to alias database."));
			break;

		case RETURN:
		case LINE_FEED:
		case 'q':
		case 'x':
		case 'r':
			error(catgets(nl_fd,NL_SETN,50,"Return to the main menu."));
			break;

		case 'p':
			error(
			    catgets(nl_fd,NL_SETN,51,"p = Check for a person in the alias database."));
			break;

		case 's':
			error(
			      catgets(nl_fd,NL_SETN,52,"s = Display system alias database."));
			break;

		case 'u':
			error(
			      catgets(nl_fd,NL_SETN,58,"u = Display user alias database."));
			break;

		case 'e':
			error(
			      catgets(nl_fd,NL_SETN,59,"e = Display fully expanded alias."));
			break;

		case ctrl('L'):
			error(catgets(nl_fd,NL_SETN,60,"^L = Rewrite screen."));
			break;

		case '.':
#ifdef SIGWINCH
			pop_state();
#endif
			return;

#ifdef SIGWINCH
		case (char)NO_OP_COMMAND:
			break;
#endif

		default:
			error( catgets(nl_fd,NL_SETN,53,"That key isn't used in this section.") );
			break;
		}

		if ( !mini_menu )
			lower_prompt( catgets(nl_fd,NL_SETN,45,"Key you want help for : ") );
		else
			lower_prompt( catgets(nl_fd,NL_SETN,47,"Key : ") );

	}
#ifdef SIGWINCH
	pop_state();
#endif
}

int
list_alias( systm )

	int		systm;

{
	/*
	 *  This display whole system or user aliases.
	 *  These are piped through 'sort' and 'more'.
	 */


	FILE            *datafile, 
			*fd_pipe;
	struct alias_rec hash_record;
	int             hashfile; 
	char            buffer[VERY_LONG_STRING], 
			fd_hash[SLEN], 
			fd_data[SLEN]; 
#ifdef SIGWINCH
	void		(*wstat) ();
	struct winsize	w_before, w_after;
#endif


	if ( systm ) {

		strcpy( fd_hash, system_hash_file );
		strcpy( fd_data, system_data_file );

	} else {

		sprintf( fd_hash, "%s/%s", getenv("HOME"), ALIAS_HASH );
		sprintf( fd_data, "%s/%s", getenv("HOME"), ALIAS_DATA );

	}

#ifdef SIGWINCH
	ioctl(0, TIOCGWINSZ, &w_before);
#endif
	sprintf( buffer, "sort|more" );

	if (( fd_pipe = popen( buffer, "w" )) == NULL )
		fd_pipe = stdout;

#ifdef SIGWINCH
	wstat = signal(SIGWINCH, SIG_IGN);
#endif

	if ( (hashfile = open(fd_hash, O_RDONLY)) > 0 ) {
		if ( (datafile = fopen(fd_data, "r")) == NULL ){ 
			printf(catgets(nl_fd,NL_SETN,56,"Opened %s hash file, but couldn't open data file!\n"),
			       systm ? "system" : "user" );
			close( hashfile );
			pclose(  fd_pipe );
#ifdef SIGWINCH
			ioctl(0, TIOCGWINSZ, &w_after);
			if ((w_before.ws_row != w_after.ws_row) ||
					(w_before.ws_col != w_after.ws_col))
				setsize(w_after.ws_row, w_after.ws_col);
			signal( SIGWINCH, wstat );
#endif
			return( -1 );
		}

		/** Otherwise let us continue... **/

		while ( read(hashfile, &hash_record, sizeof(hash_record)) != 0 ) {
			if ( strlen(hash_record.name) > 0 &&
			     !equal(hash_record.name, SIZE_INDICATOR) ) {
				fseek(datafile, hash_record.byte, 0L);
				fgets(buffer, VERY_LONG_STRING, datafile);
				fprintf(fd_pipe, "%-15s  %s", hash_record.name, buffer);
			}
		}

		fclose( datafile );
		close( hashfile );

	} else {
		pclose( fd_pipe );
#ifdef SIGWINCH
		ioctl(0, TIOCGWINSZ, &w_after);
		if ((w_before.ws_row != w_after.ws_row) ||
				(w_before.ws_col != w_after.ws_col))
			setsize(w_after.ws_row, w_after.ws_col);
		signal( SIGWINCH, wstat );
#endif
		return( 1 );
	}

	pclose( fd_pipe );
#ifdef SIGWINCH
	ioctl(0, TIOCGWINSZ, &w_after);
	if ((w_before.ws_row != w_after.ws_row) ||
			(w_before.ws_col != w_after.ws_col))
		setsize(w_after.ws_row, w_after.ws_col);
	signal( SIGWINCH, wstat );
#endif

	return(0);

}
