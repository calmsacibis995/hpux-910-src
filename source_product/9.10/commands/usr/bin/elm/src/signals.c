/**			signals.c			**/

/*
 *  @(#) $Revision: 70.7 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  This set of routines traps various signals and informs the
 *  user of the error, leaving the program in a nice, graceful
 *  manner.
 */

#include <signal.h>
#include <termio.h>

#include "headers.h"

#ifdef NLS
# define NL_SETN   46
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS

extern int      pipe_abort,	/* set to TRUE if receive SIGPIPE */
		prev_level;	/* previous softkeys level        */

void
quit_signal()
{
	dprint( 1, (debugfile, "\n\n** Received SIGQUIT **\n\n\n\n") );
	leave( 0 );
}


void
hup_signal()
{
	dprint( 1, (debugfile, "\n\n** Received SIGHUP **\n\n\n\n") );
	leave( 0 );
}


void
term_signal()

{
	dprint( 1, (debugfile, "\n\n** Received SIGTERM **\n\n\n\n") );
	leave( 0 );
}


void
ill_signal()
{
	dprint( 1, (debugfile, "\n\n** Received SIGILL **\n\n\n\n") );
	PutLine0( LINES, 0,catgets(nl_fd,NL_SETN,3, "\n\nIllegal Instruction signal!\n\n") );
	emergency_exit();
}


void
fpe_signal()
{
	dprint( 1, (debugfile, "\n\n** Received SIGFPE **\n\n\n\n") );
	PutLine0( LINES, 0,catgets(nl_fd,NL_SETN,4, "\n\nFloating Point Exception signal!\n\n" ));
	emergency_exit();
}


void
bus_signal()
{
	dprint( 1, (debugfile, "\n\n** Received SIGBUS **\n\n\n\n") );
	PutLine0( LINES, 0,catgets(nl_fd,NL_SETN,5, "\n\nBus Error signal!\n\n" ));
	emergency_exit();
}


#ifdef SIGWINCH
char	envstr1[20];
char	envstr2[20];
extern int	resized;	/* flag indicating window resize occurred */

void
setsize(lines, columns)
int	lines, columns;
{

	resized = 1;
	LINES = lines - 1;
	COLUMNS = columns;
	if (mini_menu)
		headers_per_page = LINES - 13;
	else
		headers_per_page = LINES - 8;

	/* headers_per_page must stay positive to avoid divide-by-zero
	   errors */
	if (headers_per_page <= 0)
		headers_per_page = 1;

	/* make sure the page of headers to be displayed contains
	   the current message */
	if (current >= 0)
		header_page = (current - 1)/headers_per_page;

	/* put new lines and columns values into environment */
	(void) sprintf(envstr1, "LINES=%d", lines);
	(void) putenv(envstr1);
	(void) sprintf(envstr2, "COLUMNS=%d", columns);
	(void) putenv(envstr2);
}

void
wind_signal()
{
	struct winsize	window;
	int		rc;

	rc = ioctl(0, TIOCGWINSZ, &window);
	if (rc == -1) {
		if (errno == ENOTTY)
			return;

		/* use default values if we can't get actual values */
		setsize(24, 80);
	} else
		setsize(window.ws_row, window.ws_col);
	switch(read_state()) {
	case WA_NOT:	break;
	case WA_MHM:	showscreen();
			mm_help_menu();
			break;
	case WA_AHM:	ClearScreen();
			update_title();
			last_header_page = -1;
			show_headers();
			if (mini_menu)
				show_alias_menu();
			a_help_menu();
			break;
	case WA_OHM:	display_options();
			o_help_menu();
			break;
	default:	break;
	}
	signal(SIGWINCH, wind_signal);
}
#endif

void
segv_signal()
{
	dprint( 1, (debugfile, "\n\n** Received SIGSEGV **\n\n\n\n") );
	PutLine0( LINES, 0,catgets(nl_fd,NL_SETN,6, "\n\nSegment Violation signal!\n\n" ));
	emergency_exit();
}


void
alarm_signal()
{
	/*
	 *  silently process alarm signal for timeouts... 
	 */

	void alarm_signal();

	(void) alarm( (unsigned int)0 );
	signal( SIGALRM, alarm_signal );
}


void
pipe_signal()
{
	/*
	 *  silently process pipe signal... 
	 */

	void pipe_signal();

	dprint( 2, (debugfile, "*** received SIGPIPE ***\n\n") );

	pipe_abort = TRUE;	/* internal signal   */

	signal( SIGPIPE, pipe_signal );
}


#ifdef SIGTSTP
int             was_in_raw_state;

#ifdef SIGWINCH
struct winsize	win_b_susp;
#endif

void
sig_user_stop()
{
	/*
	 * This is called when the user presses a ^Z to stop the process
	 * within BSD 
	 */

	if ( signal(SIGTSTP, SIG_DFL) != SIG_DFL ) 
		signal( SIGTSTP, SIG_DFL );

#ifdef SIGWINCH
	ioctl(0, TIOCGWINSZ, &win_b_susp);
#endif
	was_in_raw_state = RawState();
	

	if ( has_transmit )
		transmit_functions( OFF );

	if ( hp_terminal ) {
		define_softkeys( CLEAR );
		softkeys_off();
	}

	Raw( OFF );		/* turn it off regardless */

	printf( catgets(nl_fd,NL_SETN,1,"\n\nStopped.  Use \"fg\" to return to Elm\n\n") );

	kill( 0, SIGSTOP );
}


void
sig_return_from_user_stop()
{
	/*
	 *  this is called when returning from a ^Z stop 
	 */

	void sig_user_stop(), sig_return_from_user_stop();
#ifdef SIGWINCH
	struct winsize	win_a_susp;
#endif

	if ( signal(SIGTSTP, sig_user_stop) == SIG_DFL ) {
		signal( SIGCONT, sig_return_from_user_stop );
		signal( SIGTSTP, sig_user_stop );
	}

#ifdef SIGWINCH
	/* resize window if it changed while we were suspended */
	ioctl(0, TIOCGWINSZ, &win_a_susp);
	if ((win_b_susp.ws_row != win_a_susp.ws_row) ||
			 (win_b_susp.ws_col != win_a_susp.ws_col))
		 setsize(win_a_susp.ws_row, win_a_susp.ws_col);

	if ((prev_level == MAIN) && (!resized))
#else
	if ( prev_level == MAIN )
#endif
		showscreen();

	error( catgets(nl_fd,NL_SETN,2,"Back in Elm.  (you might need to explicitly request a redraw)") );

	if ( was_in_raw_state )
		Raw( ON );

	if ( has_transmit )
		transmit_functions( ON );

	if ( hp_terminal ) {
		define_softkeys( prev_level );
		softkeys_on();
	}
}
#endif
