/**			args.c				**/

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
 *  starting argument parsing routines for ELM system...
 */

#include "headers.h"


#define DONE		0
#define ERROR		-1


#ifdef NLS
# define NL_SETN   5
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


extern char    *optional_arg;	/* optional argument as we go */
extern int      opt_index;	/* argnum + 1 when we leave   */

int 
parse_arguments( argc, argv, to_whom )
	
	int             argc;
	char            *argv[], 
			*to_whom;

{
	/*
	 *  Set flags according to what was given to program.  If we are 
	 *  fed a name or series of names, put them into the 'to_whom' buffer
	 *  and set "mail_only" to TRUE 
	 */


	register int    c = 0; 


	infile[0] = '\0';
	to_whom[0] = '\0';
	batch_subject[0] = '\0';

	while ( (c = get_options(argc, argv, "?acd:f:hkKms:vwz")) > 0 ) {
		switch ( c ) {
		case 'a':
			arrow_cursor+=2;
			break;

		case 'c':
			check_only++;
			break;

		case 'd':
			debug = atoi( optional_arg );
			break;

		case 'f':
			if ( strlen(optional_arg) < LONG_FILE_NAME )
				strcpy( infile, optional_arg );
			else {
				strncpy( infile, optional_arg, LONG_FILE_NAME-1 );
				infile[LONG_FILE_NAME-1] = '\0';
			}

			mbox_specified = 2;
			break;
			
		case '?':
		case 'h':
			args_help();

		case 'k':
			hp_softkeys+=2;
			break;

		case 'K':
			hp_terminal+=2;
			hp_softkeys+=2;
			break;

		case 'm':
			mini_menu +=2;
			break;

		case 's':
			if ( strlen(optional_arg) < SLEN )
				strcpy( batch_subject, optional_arg );
			else {
				strncpy( batch_subject, optional_arg, SLEN-1 );
				batch_subject[SLEN-1] = '\0';
			}

			break;

		case 'v':
			show_version_id();

		case 'w':
			warnings = 0;
			break;

		case 'z':
			check_size++;
			break;
		}
	}

	if ( c == ERROR ) {
		printf( catgets(nl_fd,NL_SETN,1,"Usage: %s [-akKmz] [-f file] \n"), argv[0]);
		printf( catgets(nl_fd,NL_SETN,2,"       %s [-s subject] <names>\n"), argv[0]);
		printf( catgets(nl_fd,NL_SETN,3,"       %s -h\n\n"), argv[0]);
		args_help();
	}


#ifndef DEBUG
	if ( debug )
		printf( catgets(nl_fd,NL_SETN,4,"Warning: system created without debugging enabled - request ignored\n") );
	debug = 0;
#endif


	if ( opt_index < argc ) {
		while ( opt_index < argc ) {

			if ( strlen(to_whom)+strlen(argv[opt_index]) >= VERY_LONG_STRING )
				break;

			sprintf( to_whom, "%s%s%s", to_whom,
			         to_whom[0] != '\0' ? " " : "", argv[opt_index] );
			mail_only++;
			opt_index++;
		}

		check_size = 0;			/* NEVER do this if we're mailing!! */
	}

	if ( strlen(batch_subject) > 0 && !mail_only )
		exit( printf(catgets(nl_fd,NL_SETN,5,"\n\rDon't understand specifying a subject and no-one to send to!\n\n\r")) );

	if ( !mail_only && !isatty(fileno(stdin)) )
		exit( printf(catgets(nl_fd,NL_SETN,6,"\n\rDon't understand whom to send the mail to!\n\n\r")) );

	if (!isatty( fileno(stdin)) && strlen(batch_subject) == 0 && !check_only )
		strcpy( batch_subject, DEFAULT_BATCH_SUBJECT );
}


args_help()

{

	/*
	 *   print out possible starting arguments... 
	 */


	printf( catgets(nl_fd,NL_SETN,7,"\nPossible Starting Arguments for ELM program:\n\n"));
	printf( catgets(nl_fd,NL_SETN,8,"\targ\t\t\tMeaning\n"));
	printf( catgets(nl_fd,NL_SETN,9,"\t -a \t\tArrow - use the arrow pointer regardless\n"));
	printf( catgets(nl_fd,NL_SETN,10,"\t -fx\t\tFile - read file 'x' rather than mailbox\n"));
	printf( catgets(nl_fd,NL_SETN,11,"\t -h \t\tHelp - give this list of options\n"));
	printf( catgets(nl_fd,NL_SETN,12,"\t -k \t\tSoftkeys off - disable HP 2622 terminal softkeys\n") );
	printf( catgets(nl_fd,NL_SETN,13,"\t -K \t\tKeypad&softkeys off - disable use of softkeys + keypad\n"));
	printf( catgets(nl_fd,NL_SETN,14,"\t -m \t\tMenu - Turn off menu, using more of the screen\n"));
	printf( catgets(nl_fd,NL_SETN,15,"\t -sx\t\tSubject 'x' - for batchmailing\n"));
	printf( catgets(nl_fd,NL_SETN,16,"\t -z \t\tZero - don't enter Elm if no mail is pending\n"));
	printf( "\n" );
	printf( "\n" );
	exit( 1 );
}
