/**			softkeys.c			**/

/* 
 *  @(#) $Revision: 66.1 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  This file and associated routines
 */


#include "headers.h"


#ifdef FORMS_MODE_SUPPORTED
extern int	softkeys_stat;		/* status of softkeys in FORM */
#endif


#ifdef NLS
# define NL_SETN   41
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS

int 	prev_level;


int
define_softkeys( level )

	int             level;

{
	char 		cancel[SHORT],
			buffer[SLEN];

	
	cancel[0] = eof_char;
	cancel[1] = '\0';

	if ( !hp_softkeys )
		return;
	
	if ( level != CLEAR && level != CANCEL )
		prev_level = level;

	if ( level == MAIN ) {

		define_key( f1, catgets(nl_fd,NL_SETN,1,"  Read     Msg"), "\r" );
		define_key( f2, catgets(nl_fd,NL_SETN,2,"  Mail     Msg"), "m" );
		define_key( f3, catgets(nl_fd,NL_SETN,3,"  Reply  to Msg"), "r" );

		if ( user_level == 0 ) {
			define_key( f4, catgets(nl_fd,NL_SETN,4,"  Save     Msg"), "s" );
			define_key( f5, catgets(nl_fd,NL_SETN,5," Delete    Msg"), "d" );
			define_key( f6, catgets(nl_fd,NL_SETN,6,"Undelete   Msg"), "u" );
		} else {
			define_key( f4, catgets(nl_fd,NL_SETN,7," Change  Mailbox"), "c" );
			define_key( f5, catgets(nl_fd,NL_SETN,4,"  Save     Msg"), "s" );
			define_key( f6, catgets(nl_fd,NL_SETN,9," Delete/Undelete"), "^" );
		}

		define_key( f7, catgets(nl_fd,NL_SETN,10," Print     Msg"), "p" );
		define_key( f8, catgets(nl_fd,NL_SETN,11,"  Quit     Elm"), "q" );
		
	} else if ( level == ALIAS ) {
		define_key( f1, catgets(nl_fd,NL_SETN,12," Alias  Current"), "a" );
		define_key( f2, catgets(nl_fd,NL_SETN,16," Make    Alias"), "m" );
		define_key( f3, catgets(nl_fd,NL_SETN,15," Delete  Alias"), "d" );
		define_key( f4, catgets(nl_fd,NL_SETN,13," Check   Person"), "p" );
		define_key( f5, catgets(nl_fd,NL_SETN,45,"Expanded Address"), "e" );
		define_key( f6, catgets(nl_fd,NL_SETN,46," Display User"), "u" );
		define_key( f7, catgets(nl_fd,NL_SETN,14," Display System"), "s" );
		define_key( f8, catgets(nl_fd,NL_SETN,17," Return  to Main"), "r" );

	} else if ( level == YESNO ) {
		define_key( f1, catgets(nl_fd,NL_SETN,18,"  Yes"), "y" );
		clear_key( f2 );
		clear_key( f3 );
		clear_key( f4 );
		define_key( f5, catgets(nl_fd,NL_SETN,30," Cancel"), cancel );
		clear_key( f6 );
		clear_key( f7 );
		define_key( f8, catgets(nl_fd,NL_SETN,19,"   No"), "n" );

	} else if ( level == READ ) {
		define_key( f1, catgets(nl_fd,NL_SETN,20,"  Next    Page  "), " " );
		clear_key( f2 );
		define_key( f3, catgets(nl_fd,NL_SETN,21,"  Next    Msg   "), "j" );
		define_key( f4, catgets(nl_fd,NL_SETN,22,"  Prev    Msg   "), "k" );
		define_key( f5, catgets(nl_fd,NL_SETN,3,"  Reply  to Msg "), "r" );
		define_key( f6, catgets(nl_fd,NL_SETN,5," Delete    Msg   "), "d" );
		define_key( f7, catgets(nl_fd,NL_SETN,25,"  Send    Msg   "), "m" );
		define_key( f8, catgets(nl_fd,NL_SETN,17," Return  to Main "), "q" );

	} else if ( level == CHANGE ) {
		define_key( f1, catgets(nl_fd,NL_SETN,27,"  Mail  Directry"), "=/" );
		define_key( f2, catgets(nl_fd,NL_SETN,28,"  Home  Directry"), "~/" );
		clear_key( f3 );
		define_key( f4, catgets(nl_fd,NL_SETN,29,"Incoming Mailbox"), "!\n" );
		clear_key( f5 );
		clear_key( f6 );
		clear_key( f7 );
		define_key( f8, catgets(nl_fd,NL_SETN,30," Cancel"), cancel );

	} else if ( level == SAVE ) {
		define_key( f1, catgets(nl_fd,NL_SETN,27,"  Mail  Directry"), "=/" );
		define_key( f2, catgets(nl_fd,NL_SETN,28,"  Home  Directry"), "~/" );
		clear_key( f3 );
		if ( user_level != 0 )
			define_key( f4, catgets(nl_fd,NL_SETN,29,"Incoming Mailbox"), "!\n" );
		else
			clear_key( f4 );
		clear_key( f5 );
		clear_key( f6 );
		clear_key( f7 );
		define_key( f8, catgets(nl_fd,NL_SETN,30," Cancel"), cancel );

	} else if ( level == CANCEL ) {
		clear_key( f1 );
		clear_key( f2 );
		clear_key( f3 );
		clear_key( f4 );
		clear_key( f5 );
		clear_key( f6 );
		clear_key( f7 );
		define_key( f8, catgets(nl_fd,NL_SETN,30," Cancel"), cancel );

	} else if ( level == LIMIT ) {
		define_key( f1, catgets(nl_fd,NL_SETN,47,"Subject"), "subject " );
		define_key( f2, catgets(nl_fd,NL_SETN,48," From"), "from " );
		define_key( f3, catgets(nl_fd,NL_SETN,49," To"), "to " );
		clear_key( f4 );
		sprintf( buffer, "%c%s", kill_line, "all\n" );
		define_key( f5, catgets(nl_fd,NL_SETN,50,"Unlimit"), buffer );
		clear_key( f6 );
		clear_key( f7 );
		define_key( f8, catgets(nl_fd,NL_SETN,30," Cancel"), cancel );

	} else if ( level == SEND ) {
		clear_key( f5 );
		clear_key( f6 );
		clear_key( f7 );
		define_key( f8, catgets(nl_fd,NL_SETN,32," Forget"), "f" );

#ifdef FORMS_MODE_SUPPORTED
		if ( softkeys_stat == PREFORMATTED ){
			clear_key( f1 );
			define_key( f2, catgets(nl_fd,NL_SETN,33,"  Edit   Headers"), "h" );
			clear_key( f3 );
			define_key( f4, catgets(nl_fd,NL_SETN,34,"  Send"), "s" );

		} else if ( user_level == 0 ) {
			define_key( f1, catgets(nl_fd,NL_SETN,42,"  Edit    Msg"), "e" );
			define_key( f2, catgets(nl_fd,NL_SETN,43,"  Edit   Headers"), "h" );
			clear_key( f3 );
			define_key( f4, catgets(nl_fd,NL_SETN,44,"  Send"), "s" );

		} else if ( softkeys_stat == YES ){
			define_key( f1, catgets(nl_fd,NL_SETN,35,"  Edit    Form"), "e" );
			define_key( f2, catgets(nl_fd,NL_SETN,33,"  Edit   Headers"), "h" );
			clear_key( f3 );
			define_key( f4, catgets(nl_fd,NL_SETN,34,"  Send"), "s" );

		} else if ( softkeys_stat == MAYBE ){
			define_key( f1, catgets(nl_fd,NL_SETN,38,"  Edit    Msg"), "e" );
			define_key( f2, catgets(nl_fd,NL_SETN,33,"  Edit   Headers"), "h" );
			define_key( f3, catgets(nl_fd,NL_SETN,34,"  Make    Form"), "m" );
			define_key( f4, catgets(nl_fd,NL_SETN,41,"  Send"), "s" );

		} else {
#endif

			define_key( f1, catgets(nl_fd,NL_SETN,38,"  Edit    Msg"), "e" );
			define_key( f2, catgets(nl_fd,NL_SETN,33,"  Edit   Headers"), "h" );
			clear_key( f3 );
			define_key( f4, catgets(nl_fd,NL_SETN,34,"  Send"), "s" );

#ifdef FORMS_MODE_SUPPORTED
		}
#endif

	} else if ( level == CLEAR )
		reset_keys();

	softkeys_on();

}


int
define_key( key, display, send )

	int             key;
	char            *display, 
			*send;

{

	char            buffer[30];
	int		def_ch_len;


	sprintf( buffer, "%s%s", display, send );

	if ( !(def_ch_len=strlen(send)) )
		def_ch_len = -1;		/* to clear the field */

	fprintf( stderr, "%c&f%dk%dd%dL%s", ESCAPE, key,
		strlen(display), def_ch_len, buffer );
	fflush( stderr );
}


int
softkeys_on()
{
	/*
	 * enable (esc&s1A) turn on softkeys (esc&jB) and turn on MENU and
	 * USER/SYSTEM options. 
	 */


	if ( hp_softkeys ) {
		fprintf( stderr, "%c&s1A%c&jB%c&jR", ESCAPE, ESCAPE, ESCAPE );
		fflush( stderr );
	}
}


int
softkeys_off()
{
	/*
	 * turn off softkeys (esc&j@) 
	 */

	if ( hp_softkeys ) {
		fprintf( stderr, "%c&s0A%c&j@", ESCAPE, ESCAPE );
		fflush( stderr );
	}
}


int
clear_key( key )

	int	key;

{
	/*
	 *  set a key to nothing... 
	 */

	if ( hp_softkeys )
		define_key( key, "                ", "" );
}


int
reset_keys()
{
	/*
	 *  Reset user soft keys
	 */
	fprintf( stderr, "%c&f1k2a16d002L   f1           %cp", ESCAPE, ESCAPE );
	fflush( stderr );

	fprintf( stderr, "%c&f2k2a16d002L   f2           %cq", ESCAPE, ESCAPE );
	fflush( stderr );

	fprintf( stderr, "%c&f3k2a16d002L   f3           %cr", ESCAPE, ESCAPE );
	fflush( stderr );

	fprintf( stderr, "%c&f4k2a16d002L   f4           %cs", ESCAPE, ESCAPE );
	fflush( stderr );

	fprintf( stderr, "%c&f5k2a16d002L   f5           %ct", ESCAPE, ESCAPE );
	fflush( stderr );

	fprintf( stderr, "%c&f6k2a16d002L   f6           %cu", ESCAPE, ESCAPE );
	fflush( stderr );

	fprintf( stderr, "%c&f7k2a16d002L   f7           %cv", ESCAPE, ESCAPE );
	fflush( stderr );

	fprintf( stderr, "%c&f8k2a16d002L   f8           %cw", ESCAPE, ESCAPE );
	fflush( stderr );
}
