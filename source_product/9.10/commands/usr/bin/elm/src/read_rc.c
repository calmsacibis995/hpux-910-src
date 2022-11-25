/**			read_rc.c			**/

/*
 *  @(#) $Revision: 70.3 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  This file contains programs to allow the user to have a .elm/elmrc file
 *  in their home directory containing any of the following: 
 *
 *	fullname= <username string>
 *	maildir = <directory>
 *	mailbox = <file>
 *	editor  = <editor>
 *	savemail= <savefile>
 *	calendar= <calendar file name>
 *	shell   = <shell>
 *	print   = <print command>
 *	weedout = <list of headers to weed out>
 *	prefix  = <copied message prefix string>
 *	pager   = <command to use for displaying messages>
 *
 *	escape  = <single character escape, default = '~' >
 *
 * --
 *	signature = <.signature file for all outbound mail>
 * OR:
 *	localsignature = <.signature file for local mail>
 *	remotesignature = <.signature file for non-local mail>
 * --
 *
 *	bounceback= <hop count threshold, or zero to disable>
 *	timeout = <seconds for main menu timeout or zero to disable>
 *	userlevel = <0=amateur, 1=okay, 2 or greater = expert!>
 *
 *	sortby  = <sent, received, from, size, subject, mailbox>
 *
 *	alternatives = <list of addresses that forward to us>
 *
 *  and/or the logical arguments:
 *
 *	autocopy    [on|off]
 *	askcc	    [on|off]
 *	askbcc      [on|off]
 *	copy        [on|off]	
 *	expand      [on|off]
 *	resolve     [on|off]
 *	weed        [on|off]
 *	noheader    [on|off]
 *	titles      [on|off]
 *	savebyname  [on|off]
 *	skipdeleted [on|off]
 *	movepage    [on|off]
 *	pointnew    [on|off]
 *	hpkeypad    [on|off]
 *	hpsoftkeys  [on|off]
 *	alwaysleave [on|off]
 *	alwaysdel   [on|off]
 *	arrow	    [on|off]
 *	menus	    [on|off]
 *	forms	    [on|off]
 *	warnings    [on|off]
 *	names	    [on|off]
 *	ask	    [on|off]
 *	keepempty   [on|off]
 *
 *
 *  Lines starting with '#' are considered comments and are not checked
 *  any further!
 */


#include "headers.h"


#ifdef NLS
# define NL_SETN  32
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


#define NOTWEEDOUT	0
#define WEEDOUT		1
#define ALTERNATIVES	2


int
read_rc_file()

{
	/*
	 *  this routine does all the actual work of reading in the
	 *  .rc file... 
	 */


	FILE            *file;
	char            buffer[VERY_LONG_STRING], 
			filename[LONG_FILE_NAME],
	                word1[SLEN], 
			word2[LONG_FILE_NAME];
	register int    i, 
			errors = 0, 
			last = NOTWEEDOUT, 
			lineno = 0;


	sprintf( filename, "%s/%s", home, elmrcfile );

	default_weedlist();

	alternative_addresses = NULL;		/* none yet! */

top_of_this_routine:

	if ( (file = user_fopen(filename, "r")) == NULL ) {
		dprint( 2, (debugfile,
			"Warning: User has no \".elm/elmrc\" file\n\n") );

		/*
		 *  look for old-style .elmrc file too: in $HOME 
		 */

		sprintf( filename, "%s/.elm", home );

		if ( !mail_only && access(filename, 00) == -1 ) {
			get_term_chars();
			Raw( ON );
			if ( hp_terminal )
				define_softkeys( YESNO );

			printf( catgets(nl_fd,NL_SETN,1,"\n\rWarning:\n\r") );
			printf( catgets(nl_fd,NL_SETN,19,"Elm requires the directory -- $HOME/.elm to store your elmrc and alias files.\n\r") );
			printf( catgets(nl_fd,NL_SETN,20,"Do you want to create the directory, now (y/n)? y%c"), BACKSPACE );

			fflush( stdout );

			buffer[0] = (char) yesno( 'y', TRUE );
			Raw( OFF );

			if ( buffer[0] == 'y' ) {
				printf( catgets(nl_fd,NL_SETN,21,"\n\rOkay.\tI'll do it.\n\r") );
				(void) sleep( 3 );
				sprintf( buffer, "%s/.elm", home );
				if ( mkdir( buffer, 0700 ) != 0 ) {
					printf( catgets(nl_fd,NL_SETN,22,"\n\rCouldn't make the directory.   * %s *\n\r"),
						error_name(errno) );
					leave( errno );
				}
			} else if ( buffer[0] == 'n' ) {
				printf( catgets(nl_fd,NL_SETN,23,"\n\rOkay.\tBut, you may have some problems later.\n\r") );
				(void) sleep( 3 );
			} else
				leave( 0 );
		}

		sprintf( filename, "%s/.elmrc", home );

		if ( !mail_only && access(filename, 00) != -1 ) {
			get_term_chars();
			Raw( ON );
			if ( hp_terminal )
				define_softkeys( YESNO );

			printf( catgets(nl_fd,NL_SETN,1,"\n\rWarning:\n\r") );
			printf( catgets(nl_fd,NL_SETN,2,"All the Elm 'dot' files are expected to have been moved into their own\n\r") );
			printf( catgets(nl_fd,NL_SETN,3,"directory -- $HOME/.elm -- by now.  Would you like me to move them all\n\r") );
			printf( catgets(nl_fd,NL_SETN,4,"for you [if you choose not to, the files will be ignored] (y/n)? y%c"), BACKSPACE );

			fflush( stdout );

			buffer[0] = (char) yesno( 'y', TRUE );
			Raw( OFF );

			if ( buffer[0] == 'y' ) {
				printf( catgets(nl_fd,NL_SETN,21,"\n\rOkay.\tI'll do it.\n\r") );
				(void) sleep( 3 );
				move_old_files_to_new();
				goto top_of_this_routine;
			} else if ( buffer[0] == eof_char )
				leave( 0 );
			else {
				printf( catgets(nl_fd,NL_SETN,5,"\n\rI've left them.\n\r") );
				(void) sleep( 3 );
			}
		}
	
		if ( !mail_only && access(folders, 00) == -1 ) {
			get_term_chars();
			Raw( ON );
			if ( hp_terminal )
				define_softkeys( YESNO );

			printf( catgets(nl_fd,NL_SETN,1,"\n\rWarning:\n\r") );
			printf( catgets(nl_fd,NL_SETN,24,"Couldn't find the folder directory -- %s\n\r"),
				folders );
			printf( catgets(nl_fd,NL_SETN,20,"Do you want to create the directory, now (y/n)? y%c"), BACKSPACE );

			fflush( stdout );

			buffer[0] = (char) yesno( 'y', TRUE );
			Raw( OFF );

			if ( buffer[0] == 'y' ) {
				printf( catgets(nl_fd,NL_SETN,21,"\n\rOkay.\tI'll do it.\n\r") );
				(void) sleep( 3 );
				if ( mkdir( folders, 0700 ) != 0 ) {
					printf( catgets(nl_fd,NL_SETN,22,"\n\rCouldn't make the directory.   * %s *\n\r"),
						error_name(errno) );
					leave( errno );
				}
			} else if ( buffer[0] == 'n' ) {
				printf( catgets(nl_fd,NL_SETN,23,"\n\rOkay.\tBut, you may have some problems later.\n\r") );
				(void) sleep( 3 );
			} else
				leave( 0 );
		}

		return;				/* we're done with this bit, at least! */
	}

	while ( fgets(buffer, VERY_LONG_STRING, file) != NULL ) {
		lineno++;
		no_ret( buffer );			/* remove return */

		if ( buffer[0] == '#' ) {		/* comment       */
			last = NOTWEEDOUT;
			continue;
		}

		if ( strlen(buffer) < 2 ) {		/* empty line    */
			last = NOTWEEDOUT;
			continue;
		}

		breakup( buffer, word1, word2 );

		strcpy( word1, shift_lower(word1) );	/* to lower case */

		if ( word2[0] == 0 && (last != WEEDOUT || last != ALTERNATIVES) ) {
			dprint( 2, (debugfile,
				"Error: Bad .elm/elmrc entry in users file;\n-> \"%s\"\n",
				buffer) );
			fprintf( stderr, catgets(nl_fd,NL_SETN,6,"Line %d of your \".elm/elmrc\" is badly formed:\n> %s\n"),
				 lineno, buffer );
			errors++;
			continue;
		}

		if ( equal(word1, "maildir") || equal(word1, "folders") ) {
			expand_env( folders, word2 );
			last = NOTWEEDOUT;
		} else if ( equal(word1, "fullname") || equal(word1, "username") ||
			   equal(word1, "name") ) {
			strcpy( full_username, word2 );
			last = NOTWEEDOUT;
		} else if ( equal(word1, "prefix") ) {
			for ( i = 0; i < strlen(word2); i++ )
				prefixchars[i] = ( word2[i] == '_' ? ' ' : word2[i] );
			prefixchars[i] = '\0';
			last = NOTWEEDOUT;
		} else if ( equal(word1, "nameserver") ) {
			expand_env( nameserver, word2 );
			last = NOTWEEDOUT;
		} else if ( equal(word1, "shell") ) {
			expand_env( shell, word2 );
			last = NOTWEEDOUT;
		} else if ( equal(word1, "sort") || equal(word1, "sortby") ) {
			strcpy( word2, shift_lower(word2) );
			if ( equal(word2, "sent") )
				sortby = SENT_DATE;
			else if ( equal(word2, "received") || equal(word2, "recieved") )
				sortby = RECEIVED_DATE;
			else if ( equal(word2, "from") || equal(word2, "sender") )
				sortby = SENDER;
			else if ( equal(word2, "size") || equal(word2, "lines") )
				sortby = SIZE;
			else if ( equal(word2, "subject") )
				sortby = SUBJECT;
			else if ( equal(word2, "status") )
				sortby = STATUS;
			else if ( equal(word2, "mailbox") || equal(word2, "folder") )
				sortby = MAILBOX_ORDER;
			else if ( equal(word2, "reverse-sent") || equal(word2, "rev-sent") )
				sortby = -SENT_DATE;
			else if ( first_word(word2, "reverse-rec") || first_word(word2, "rev-rec") )
				sortby = -RECEIVED_DATE;
			else if ( equal(word2, "reverse-from") || equal(word2, "rev-from")
				 		|| equal(word2, "reverse-sender") 
				 		|| equal(word2, "rev-sender") )
				sortby = -SENDER;
			else if ( equal(word2, "reverse-size") || equal(word2, "rev-size")
				 		|| equal(word2, "reverse-lines") 
				 		|| equal(word2, "rev-lines") )
				sortby = -SIZE;
			else if ( equal(word2, "reverse-subject") ||
				 equal(word2, "rev-subject") )
				sortby = -SUBJECT;
			else if ( equal(word2, "reverse-status") ||
				  equal(word2, "rev-status") )
				sortby = -STATUS;
			else if ( equal(word2, "reverse-mailbox") ||
				 equal(word2, "rev-mailbox") ||
				 equal(word2, "reverse-folder") ||
				 equal(word2, "rev-folder") )
				sortby = -MAILBOX_ORDER;
			else {
				if ( !errors )
					printf( catgets(nl_fd,NL_SETN,7,"Error reading '.elm/elmrc' file;\n") );
				printf( catgets(nl_fd,NL_SETN,8,"Line %d: Don't know what sort key '%s' specifies!\n"),
				       lineno, word2 );
				errors++;
				continue;
			}
		} else if ( equal(word1, "mailbox") ) {
			expand_env( mailbox, word2 );
			last = NOTWEEDOUT;
		} else if ( equal(word1, "editor") || equal(word1, "mailedit") ) {
			expand_env( editor, word2 );
			last = NOTWEEDOUT;
		} else if ( equal(word1, "savemail") || equal(word1, "saveto") ) {
			expand_env( savefile, word2 );
			last = NOTWEEDOUT;
		} else if ( equal(word1, "calendar") ) {
			expand_env( calendar_file, word2 );
			last = NOTWEEDOUT;
		} else if ( equal(word1, "print") || equal(word1, "printmail") ) {
			expand_env( printout, word2 );
			last = NOTWEEDOUT;
		} else if ( equal(word1, "pager") || equal(word1, "page") ) {
			expand_env( pager, word2 );
			if ( equal(pager, "builtin+") || equal(pager, "internal+") )
				clear_pages++;
			last = NOTWEEDOUT;
		} else if ( equal(word1, "signature") ) {
			if ( equal(shift_lower(word2), "on") ||
			    equal(shift_lower(word2), "off") ) {
				fprintf( stderr,
				catgets(nl_fd,NL_SETN,9,"\"signature\" used in obsolete way in .elm/elmrc file - ignored!\n\r") );
				fprintf( stderr,
				catgets(nl_fd,NL_SETN,10,"\t(signature should specify the filename to use rather than on/off)\n\r\n") );
			} else {
				expand_env( local_signature, word2 );
				strcpy( remote_signature, local_signature );
				signature = TRUE;
			}
			last = NOTWEEDOUT;
		} else if ( equal(word1, "localsignature") ) {
			expand_env( local_signature, word2 );
			signature = TRUE;
			last = NOTWEEDOUT;
		} else if ( equal(word1, "remotesignature") ) {
			expand_env( remote_signature, word2 );
			signature = TRUE;
			last = NOTWEEDOUT;
		} else if ( equal(word1, "escape") ) {
			escape_char = word2[0];
			last = NOTWEEDOUT;
		} else if ( equal(word1, "autocopy") ) {
			auto_copy = is_it_on( word2 );
			last = NOTWEEDOUT;
		} else if ( equal(word1, "copy") || equal(word1, "auto_cc") ) {
			auto_cc = is_it_on( word2 );
			last = NOTWEEDOUT;
		} else if ( equal(word1, "names") ) {
			names_only = is_it_on( word2 );
			last = NOTWEEDOUT;
		} else if ( equal(word1, "resolve") ) {
			resolve_mode = is_it_on( word2 );
			last = NOTWEEDOUT;
		} else if ( equal(word1, "weed") ) {
			filter = is_it_on( word2 );
			last = NOTWEEDOUT;
		} else if ( equal(word1, "noheader") ) {
			noheader = is_it_on( word2 );
			last = NOTWEEDOUT;
		} else if ( equal(word1, "titles") ) {
			title_messages = is_it_on( word2 );
			last = NOTWEEDOUT;
		} else if ( equal(word1, "savebyname") || equal(word1, "savename") ) {
			save_by_name = is_it_on( word2 );
			last = NOTWEEDOUT;
		} else if ( equal(word1, "movepage") || equal(word1, "page") ||
			   equal(word1, "movewhenpaged") ) {
			move_when_paged = is_it_on( word2 );
			last = NOTWEEDOUT;
		} else if ( equal(word1, "pointnew") || equal(word1, "pointtonew") ) {
			point_to_new = is_it_on( word2 );
			last = NOTWEEDOUT;
		} else if ( equal(word1, "keypad") || equal(word1, "hpkeypad") ) {
			if ( hp_terminal <= 1 )
				hp_terminal = is_it_on( word2 );
			else
				hp_terminal = FALSE;
			last = NOTWEEDOUT;
		} else if ( equal(word1, "softkeys") || equal(word1, "hpsoftkeys") ) {
			if ( hp_softkeys <= 1 )
				hp_softkeys = is_it_on(word2);
			else
				hp_softkeys = FALSE;
			last = NOTWEEDOUT;
		} else if ( equal(word1, "arrow") ) {
			if ( arrow_cursor <= 1 )
				arrow_cursor = is_it_on( word2 );
			last = NOTWEEDOUT;
		} else if ( first_word(word1, "form") ) {
			allow_forms = ( is_it_on(word2) ? MAYBE : NO );
			last = NOTWEEDOUT;
		} else if ( first_word(word1, "menu") ) {
			if ( mini_menu <= 1 )
				mini_menu = is_it_on( word2 );
			else
				mini_menu = FALSE;
			last = NOTWEEDOUT;
		} else if ( first_word(word1, "warning") ) {
			warnings = is_it_on( word2 );
			last = NOTWEEDOUT;
		} else if ( equal(word1, "alwaysleave") || equal(word1, "leave") ) {
			always_leave = is_it_on( word2 );
			last = NOTWEEDOUT;
		} else if ( equal(word1, "alwaysdelete") || equal(word1, "delete") ) {
			always_del = is_it_on( word2 );
			last = NOTWEEDOUT;
		} else if ( equal(word1, "askcc") || equal(word1, "cc") ) {
			prompt_for_cc = is_it_on( word2 );
			last = NOTWEEDOUT;
		} else if ( equal(word1, "askbcc") || equal(word1, "bcc") ) {
			prompt_for_bcc = is_it_on( word2 );
			last = NOTWEEDOUT;
		} else if ( equal(word1, "ask") || equal(word1, "question") ) {
			question_me = is_it_on( word2 );
			last = NOTWEEDOUT;
		} else if ( equal(word1, "keep") || equal(word1, "keepempty") ) {
			keep_empty_files = is_it_on( word2 );
			last = NOTWEEDOUT;
		} else if ( equal(word1, "expand") || equal(word1, "expandtabs") ) {
			expand_tabs = is_it_on( word2 );
			last = NOTWEEDOUT;
		} else if ( equal(word1, "skipdeleted") || equal(word1, "skip_deleted") ) {
			skip_deleted = is_it_on( word2 );
			last = NOTWEEDOUT;


		} else if ( equal(word1, "bounce") || equal(word1, "bounceback") ) {
#ifdef BOUNCEBACK_ENABLED

			bounceback = atoi( word2 );
			if ( bounceback > MAX_HOPS ) {
				fprintf( stderr,
				catgets(nl_fd,NL_SETN,11,"Warning: bounceback is set to greater than %d (max-hops) - Ignored.\n"),
				MAX_HOPS );
				bounceback = 0;
			}

#endif
			last = NOTWEEDOUT;


		} else if ( equal(word1, "userlevel") ) {
			user_level = atoi( word2 );
			last = NOTWEEDOUT;
		} else if ( equal(word1, "timeout") ) {
			timeout = atoi( word2 );
			if ( timeout < 10 ) {
				fprintf( stderr,
				catgets(nl_fd,NL_SETN,12,"Warning: timeout is set to less than 10 seconds - Ignored.\n") );
				timeout = 0;
			}
			last = NOTWEEDOUT;
		} else if ( equal(word1, "weedout") ) {
			weedcount = 0;
			weedout( word2 );
			last = WEEDOUT;
		} else if ( equal(word1, "alternatives") ) {
			alternatives( word2 );
			last = ALTERNATIVES;
		} else if ( last == WEEDOUT )		/* could be multiple line
						 	 * weedout */
			weedout( buffer );
		else if ( last == ALTERNATIVES )	/* multi-line addresses   */
			alternatives( buffer );
		else {
			fprintf( stderr,
			catgets(nl_fd,NL_SETN,13,"I can't understand line %d in your \".elm/elmrc\" file:\n> %s\n"),
			lineno, buffer );

			/*
			 * no error flagged -- let use continue, ok? 
			 */

			(void) sleep( 2 );
		}
	}

	user_fclose( file );


#ifdef DEBUG

	if ( debug > 10 )		/** only do this if we REALLY want debug! **/
		dump_rc_results();

#endif DEBUG
	
	if ( !mail_only && access(folders, 00) == -1 ) {
		get_term_chars();
		Raw( ON );
		if ( hp_terminal )
			define_softkeys( YESNO );

		printf( catgets(nl_fd,NL_SETN,1,"\n\rWarning:\n\r") );
		printf( catgets(nl_fd,NL_SETN,24,"Couldn't find the folder directory -- %s\n\r"),
			folders );
		printf( catgets(nl_fd,NL_SETN,20,"Do you want to create the directory, now (y/n)? y%c"), BACKSPACE );

		fflush( stdout );

		buffer[0] = (char) yesno( 'y', TRUE );
		Raw( OFF );

		if ( buffer[0] == 'y' ) {
			printf( catgets(nl_fd,NL_SETN,21,"\n\rOkay.\tI'll do it.\n\r") );
			(void) sleep( 3 );
			if ( mkdir( folders, 0700 ) != 0 ) {
				printf( catgets(nl_fd,NL_SETN,22,"\n\rCouldn't make the directory.   * %s *\n\r"),
					error_name(errno) );
				leave( errno );
			}
		} else if ( buffer[0] == 'n' ) {
			printf( catgets(nl_fd,NL_SETN,23,"\n\rOkay.\tBut, you may have some problems later.\n\r") );
			(void) sleep( 3 );
		} else
			leave( 0 );
	}

	if ( errors )
		leave( errors );
}


int
weedout( string )

	char           *string;

{
	/*
	 *  This routine is called with a list of headers to weed out.   
	 */


	char            *strptr, 
			*header;
	register int    i,
			spaces;


	strptr = string;

	while ( (header = strtok(strptr, "\t,\"'")) != NULL) {
		if (strlen(header) > 0 ) {
			if ( weedcount > MAX_IN_WEEDLIST ) {
				fprintf( stderr, catgets(nl_fd,NL_SETN,14,"Too many weed headers!  Leaving...\n") );
				leave( 1 );
			}

			weedlist[weedcount] = my_malloc(strlen(header)+1,
							"weedlist");

			for ( i = 0; i < strlen(header); i++ )
				if ( header[i] == '_' )
					header[i] = ' ';
			spaces = TRUE;
			for ( i = 0; i < strlen(header); i++ )
				if ( header[i] != ' ' )
					spaces = FALSE;

			if ( !spaces ) {
				strcpy( weedlist[weedcount], header );
				weedcount++;
			}
		}
		strptr = NULL;
	}
}


int
alternatives( string )

	char           *string;

{
	/*
	 *  This routine is called with a list of alternative addresses
	 *  that you may receive mail from (forwarded) 
	 */


	char            *strptr, 
			*address;
	struct addr_rec *current_record, 
			*previous_record;


	previous_record = alternative_addresses;	/* start 'er up! */

	/*
	 * move to the END of the alternative addresses list 
	 */

	if ( previous_record != NULL )
		while ( previous_record->next != NULL )
			previous_record = previous_record->next;

	strptr = (char *) string;

	while ( (address = strtok(strptr, "\t ,\"'")) != NULL ) {

		if ( previous_record == NULL ) {
			previous_record =
			    (struct addr_rec *)
				my_malloc( sizeof(*alternative_addresses),
					   "address" );

			strcpy( previous_record->address, address );
			previous_record->next = NULL;
			alternative_addresses = previous_record;
		} else {
			current_record =
			    (struct addr_rec *)
				my_malloc( sizeof(*alternative_addresses),
					   "address" );

			strcpy( current_record->address, address );
			current_record->next = NULL;
			previous_record->next = current_record;
			previous_record = current_record;
		}

		strptr = (char *) NULL;
	}
}


int
default_weedlist()

{
	/*
	 *  Install the default headers to weed out!  Many gracious 
	 *  thanks to John Lebovitz for this dynamic method of 
	 *  allocation!
	 */


	static char    *default_list[] = { ">From", "In-Reply-To:",
				  "References:", "Newsgroups:", "Received:",
			   	  "Apparently-To:", "Message-Id:", "Content-Type:",
				  "From", "Mailer:", NULL
				};


	for ( weedcount = 0; default_list[weedcount] != (char *) 0; weedcount++ ) {

		weedlist[weedcount] =
			my_malloc(strlen(default_list[weedcount]) + 1,
				  "weedlist");

		strcpy( weedlist[weedcount], default_list[weedcount] );
	}
}


int
matches_weedlist( buffer )

	char           *buffer;

{
	/*
	 *  returns true iff the first 'n' characters of 'buffer' 
	 *  match an entry of the weedlist 
	 */


	register int    i;


	for ( i = 0; i < weedcount; i++ )
		if ( first_word(buffer, weedlist[i]) )
			return ( 1 );

	return ( 0 );
}


int
breakup( buffer, word1, word2 )

	char            *buffer, 
			*word1, 
			*word2;

{
	/*
	 *  This routine breaks buffer down into word1, word2 where 
	 *  word1 is alpha characters only, and there is an equal
	 *  sign delimiting the two...
	 *	alpha = beta
	 *  For lines with more than one 'rhs', word2 is set to the
	 *  entire string...
	 */


	register int    i;


	for ( i = 0; buffer[i] != '\0' && ok_char(buffer[i]); i++ )
		if ( buffer[i] == '_' )
			word1[i] = '-';
		else if ( isupper(buffer[i]) )
			word1[i] = (char)tolower( (int)buffer[i] );
		else
			word1[i] = buffer[i];

	word1[i++] = '\0';			/* that's the first word! */

	/*
	 *  spaces before equal sign? 
	 */

	while ( whitespace(buffer[i]) )
		i++;

	if ( buffer[i] == '=' )
		i++;

	/*
	 *  spaces after equal sign? 
	 */

	while ( whitespace(buffer[i]) )
		i++;

	if ( buffer[i] != '\0' )
		strcpy( word2, (char *) (buffer + i) );
	else
		word2[0] = '\0';

	while ( whitespace(word2[strlen(word2) - 1]) )
		word2[strlen(word2) - 1] = '\0';

	if ( word2[0] == '"' && word2[strlen(word2) - 1] == '"' ) {
		word2[strlen(word2) - 1] = '\0';
		strcpy( word2, (char *) (word2 + 1) );
	}

}


int
expand_env( dest, buffer )

	char            *dest, 
			*buffer;

{
	/*
	 *  expand possible metacharacters in buffer and then copy
	 *  to dest... 
	 *  This routine knows about "~" being the home directory,
	 *  and "$xxx" being an environment variable.
	 */


	char            *word, 
			*string, 
			next_word[LONG_FILE_NAME];


	if ( buffer[0] == '/' ) {
		dest[0] = '/';
		dest[1] = '\0';
	} else
		dest[0] = '\0';

	string = (char *) buffer;

	while ( (word = strtok(string, "/")) != NULL ) {

		if ( word[0] == '$' ) {
			next_word[0] = '\0';
			if ( getenv((char *) (word + 1)) != NULL )
				strcpy( next_word, getenv((char *) (word + 1)) );
			if ( strlen(next_word) == 0 ) {
				fprintf(stderr, catgets(nl_fd,NL_SETN,17,"\n\rCan't expand environment variable '%s'\n\r"),
				       word);
				leave( 1 );
			}

		} else if ( word[0] == '~' && word[1] == '\0' )
			strcpy( next_word, home );

		else
			strcpy( next_word, word );

		if ( strlen(dest)+strlen(next_word)+1 > LONG_FILE_NAME )
			fprintf( stderr, catgets(nl_fd,NL_SETN,18,"\n\rCan't expand environment variable '%s' - too long filename"), buffer );
		else
			sprintf( dest, "%s%s%s", dest,
			(strlen(dest) > 0 && lastch(dest) != '/' ? "/" : ""),
			next_word );

		string = (char *) NULL;
	}
}


#ifdef DEBUG

#define on_off(s)	( s == 1? "ON " : "OFF" )


int
dump_rc_results()

{

	register int    i, 
			len;


	fprintf( debugfile, "folders = %s ", folders );
	fprintf( debugfile, "mailbox = %s ", mailbox );
	fprintf( debugfile, "editor = %s\n", editor );
	fprintf( debugfile, "printout = %s ", printout );
	fprintf( debugfile, "savefile = %s ", savefile );
	fprintf( debugfile, "calendar_file = %s\n", calendar_file );
	fprintf( debugfile, "prefixchars = %s ", prefixchars );
	fprintf( debugfile, "shell = %s ", shell );
	fprintf( debugfile, "pager = %s\n", pager );
	fprintf( debugfile, "\n" );
	fprintf( debugfile, "escape = %c\n", escape_char );
	fprintf( debugfile, "\n" );

	fprintf( debugfile, "mini_menu    = %s ", on_off(mini_menu) );
	fprintf( debugfile, "mbox_specified = %s ", on_off(mbox_specified) );
	fprintf( debugfile, "check_first    = %s ", on_off(check_first) );
	fprintf( debugfile, "auto_copy    = %s\n", on_off(auto_copy) );

	fprintf( debugfile, "filter_hdrs  = %s ", on_off(filter) );
	fprintf( debugfile, "resolve_mode   = %s ", on_off(resolve_mode) );
	fprintf( debugfile, "auto_save_copy = %s ", on_off(auto_cc) );
	fprintf( debugfile, "noheader     = %s\n", on_off(noheader) );

	fprintf( debugfile, "title_msgs   = %s ", on_off(title_messages) );
	fprintf( debugfile, "hp_terminal    = %s ", on_off(hp_terminal) );
	fprintf( debugfile, "hp_softkeys    = %s ", on_off(hp_softkeys) );
	fprintf( debugfile, "save_by_name = %s\n", on_off(save_by_name) );

	fprintf( debugfile, "move_paged   = %s ", on_off(move_when_paged) );
	fprintf( debugfile, "point_to_new   = %s ", on_off(point_to_new) );
	fprintf( debugfile, "bounceback     = %s ", on_off(bounceback) );
	fprintf( debugfile, "signature    = %s\n", on_off(signature) );

	fprintf( debugfile, "always_leave = %s ", on_off(always_leave) );
	fprintf( debugfile, "always_delete  = %s ", on_off(always_del) );
	fprintf( debugfile, "arrow_cursor   = %s ", on_off(arrow_cursor) );
	fprintf( debugfile, "names        = %s\n", on_off(names_only) );

	fprintf( debugfile, "warnings     = %s ", on_off(warnings) );
	fprintf( debugfile, "question_me    = %s ", on_off(question_me) );
	fprintf( debugfile, "keep_nil_files = %s\n\n",
		on_off(keep_empty_files) );

	fprintf( debugfile, "Userlevel is set to %s user: %d\n",
		user_level == 0 ? "beginning" :
		user_level > 1 ? "expert" : "intermediate", user_level );

	fprintf( debugfile, "\nAnd we're skipping the following headers:\n\t" );

	for ( len = 8, i = 0; i < weedcount; i++ ) {
		if ( weedlist[i][0] == '*' )
			continue;	/* skip '*end*' */
		if ( len + strlen(weedlist[i]) > 80 ) {
			fprintf( debugfile, " \n\t" );
			len = 8;
		}
		fprintf( debugfile, "%s  ", weedlist[i] );
		len += strlen( weedlist[i] ) + 3;
	}

	fprintf( debugfile, "\n\n" );
}


#endif DEBUG


int
is_it_on( word )

	char           *word;

{
	/*
	 *  Returns TRUE if the specified word is either 'ON', 'YES'
	 *  or 'TRUE', and FALSE otherwise.   We explicitly translate
	 *  to lowercase here to ensure that we have the fastest
	 *  routine possible - we really DON'T want to have this take
	 *  a long time or our startup will be major pain each time.
	 */


	static char     mybuffer[NLEN];
	register int    i, 
			j;


	for ( i = 0, j = 0; word[i] != '\0'; i++ )
		mybuffer[j++] = isupper(word[i]) 
				? (char)tolower((int)word[i]) : word[i];

	mybuffer[j] = '\0';

	return ( first_word(mybuffer, "on")  ||
		 first_word(mybuffer, "yes") ||
		 first_word(mybuffer, "true")  );
}
