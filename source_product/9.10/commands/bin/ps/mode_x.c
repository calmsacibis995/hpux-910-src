#include "monitor.h"
static char *HPUX_ID = "$Revision: 70.1 $";

/* List monitor mode/command keys */
mode_x()
{
	in_screen = 0;

	atf(0, 2, "\
  Monitor mode keys:			  Other monitor commands:\n\
  ------------------			  -----------------------\n\
  C - Configuration values / Drivers	  ? - Display help for current screen\n\
  G - Global system status		  ! - Invoke a shell\n\
  I - I/O status			  B - Backward to previous screen (or -)\n\
  K - Diskless status			  D - Dump screen to hardcopy\n\
  L - LAN status			  F - Forward to next screen (' ' or +)\n\
  M - Message protocol status		  H - Halt monitor updates\n\
  N - Networked status			  P - Select a process to analyze\n\
  O - Message opcode statistics		  Q - Quit monitor\n\
  R - Remote uptime			  R - Resume from HALTED state\n\
  S - Single process information	  U - Update configuration variables\n\
  T - Tasks running			  X - Return to main menu\n\
  V - Virtual memory status		 ^L - Re-draw screen\n\
\n\
\n\
\n\
For a list of command line options, invoke as: 'monitor help'\n\
--------------------------------------------------------------------------------");

	if (osdesignermode) {
		/* HIDDEN KEYS FOR NORMAL USERS */
		atf(41, 16, "^O - Enter OS designer mode");
		atf(41, 17, "^U - Return to user mode");
	}

	/* Indicate monitor state variables */
	atf(0,  21, "Hard copy device (for /usr/bin/lp): %s\n", hardcopy);
	addf(	    "Update interval = %d seconds\n", update_interval);
	addf(	    "N screen protocol revision ID = %s", NREVISION);
	atf(60, 22, &whatstring[4]);    /* sccs id string */
	atf(61, 23, &buildstring[4]);   /* string describing system built on */
}
