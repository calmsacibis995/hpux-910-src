/*****************************************************************************
** Copyright (c)        1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: tutils.c,v 70.1 92/03/09 16:14:49 ssa Exp $ */
/*
 * File:		tutils.c
 * Creator:		G.Clark.Brown
 *
 * Curses, etc. utility routines for the Terminfo Exerciser Program.
 *
 * Module name: 	%M%
 * SID:			%I%
 * Extract date:	%H% %T%
 * Delta date:		%G% %U%
 * Stored at:		%P%
 * Module type:		%Y%
 * 
 */

#define		M_TERMINFO
#include	<curses.h>
#include	<string.h>
#include	<errno.h>
#include	<term.h>

#include	"tixermacro.h"
#include	"tixer.h"
#include	"tixerext.h"

#ifndef lint
	static char	Sccsid[]="%W% %E% %G%";
#endif

extern int	errno;
extern int	sys_nerr;
extern char	*sys_errlist[];

/* = = = = = = = = = = = = = = = open_term = = = = = = = = = = = = = = = =*/
/*
 * Call initscr(3) and check for errors.
 */

static SCREEN	*Screen=NULL;
#ifdef UNI90
extern SCREEN	*newterm();
#endif

open_term(term)

char	*term;

{


	if ((Screen=newterm(term, stdout, stdin))==NULL)
	{
		if ( errno == ENOENT )
			printf( "Invalid TERM variable\r\n" );
		else
		{
			perror("Error in newterm call:");
			printf("\r\n");
		}
		u_term(-1);
	}
	set_term(Screen);
	
	
	def_prog_mode();
	
	keypad(stdscr, TRUE);

	cbreak();
	noecho();
	nonl();

	return 0;
}

/* = = = = = = = = = = = = = = = close_term = = = = = = = = = = = = = = = =*/
/*
 * Call endwin(3).
 */

close_term()

{
	if ( Screen != NULL )
		endwin();

	return;
}

/* = = = = = = = = = = = = = tu_pranykey = = = = = = = = = = = = = = =*/
/*
 * Ask the user to "Press any key to continue" and wait.
 */

tu_pranykey(p_opts)

struct option_tab	*p_opts;

{
	if(at_pause(p_opts))
		return;
	printf( "Press any key to continue.");
	fflush(stdout);
	getchar();
	return;
}

/* = = = = = = = = = = = = = term_cursor_pattern = = = = = = = = = = = = = = =*/
/*
 * Generate a test pattern of lines and columns to test cursor positioning.
 */

term_cursor_pattern()

{
	erase();
	refresh();
	attrset(0);

	box(stdscr, 0, 0);

	mvaddstr(9, 58, "Line 9, column 58.");
	refresh();

	mvaddstr(5, 3, "Line 5, column 3.");
	refresh();

	mvaddstr(12, 43, "Line 12, column 43.");
	refresh();

	mvaddstr(15, 13, "Line 15, column 13.");
	refresh();

	mvaddstr(1, 2, "Line 1, column 2.");
	refresh();

	move( LINES - 2, 2 );
	refresh();

	return 0;
}

/* = = = = = = = = = = = = = term_attrib_pattern = = = = = = = = = = = = = = =*/
/*
 * Generate a test pattern of attributes.
 */

term_attrib_pattern()

{
	int	i;
	static int	Attr_rev[ 2 ] =		{ A_NORMAL, A_REVERSE };
	static char	*Name_rev[ 2 ] =	{ " NORM  ", " REVER " };
	static int	Attr_int[ 3 ] =		{ A_NORMAL, A_DIM, A_BOLD };
	static char	*Name_int[ 3 ] =	{ "       ", " DIM   ",
								" BOLD  " };
	static int	Attr_blink[ 2 ] =	{ A_NORMAL, A_BLINK };
	static char	*Name_blink[ 2 ] =	{ "       ", " BLINK " };
	static int	Attr_under[ 2 ] =	{ A_NORMAL, A_UNDERLINE };
	static char	*Name_under[ 2 ] =	{ "       ", " UNDER "};
	int	j;
	int	k;
	int	l;
	int	line;
	int	col;
	int	top;

	erase();
	refresh();
	box( stdscr, 0, 0 );
	refresh();
	line = 2;
	attrset( A_ALTCHARSET );
	mvprintw( line, 1, "%s", " ALTCH`abcdefghijklmonpqrstuvwxyz{|}~" );
	attrset( A_ALTCHARSET | A_REVERSE );
	mvprintw( line, 40, "%s", " ALTCH`abcdefghijklmonpqrstuvwxyz{|}~" );
	line++;
	line++;
	attrset( A_ALTCHARSET | A_DIM );
	mvprintw( line, 1, "%s", " ALTCH`abcdefghijklmonpqrstuvwxyz{|}~" );
	attrset( A_ALTCHARSET | A_BOLD );
	mvprintw( line, 40, "%s", " ALTCH`abcdefghijklmonpqrstuvwxyz{|}~" );
	line++;
	line++;
	attrset( A_ALTCHARSET | A_BLINK );
	mvprintw( line, 1, "%s", " ALTCH`abcdefghijklmonpqrstuvwxyz{|}~" );
	attrset( A_ALTCHARSET | A_UNDERLINE );
	mvprintw( line, 40, "%s", " ALTCH`abcdefghijklmonpqrstuvwxyz{|}~" );
	line++;
	line++;
	attrset( 0 );
	refresh();
	top = line;
	col = 2;
	for ( i = 0; i < 2; i++ ) /* NORMAL - REVERSE */
	{
	    line = top;
	    for ( j = 0; j < 3; j++ ) /* NORMAL - BOLD - DIM */
	    {
		for ( k = 0; k < 2; k++ ) /* NORMAL - BLINK */
		{
		    for ( l = 0; l < 2; l++ ) /* NORMAL - UNDER */
		    {
			attrset( Attr_rev[ i ]   | Attr_int[ j ]
			       | Attr_blink[ k ] | Attr_under[ l ] );
			mvprintw( line, col, "%s%s%s%s",
				Name_rev[ i ], Name_int[ j ],
				Name_blink[ k ], Name_under[ l ] );
			refresh();
			line++;
		    }
		}
	    }
	    col = 40;
	}
	attrset( A_STANDOUT );
	mvprintw( 21, 2, "STANDOUT" );
	attrset( A_PROTECT );
	mvprintw( 21, 40, "PROTECT" );
	attrset( 0 );
	move( LINES - 2, 2 );
	refresh();

	return 0;
}


/* = = = = = = = = = = = = = print_cap_title = = = = = = = = = = = = = = =*/
/*
 * Print title of a capability.
 */

print_cap_title(p_opts)

struct option_tab	*p_opts;

{
	printf(
		"=== %25.25s === %8.8s === %2.2s ===\r\n======: %s\r\n",
		cap_table[p_opts->captab].variable_name,
		cap_table[p_opts->captab].capname,
		cap_table[p_opts->captab].icode,
			cap_table[p_opts->captab].description);
}


/* = = = = = = = = = = = = = tu_clear_screen = = = = = = = = = = = = = = =*/
/*
 * Find some way to clear the terminal screen.
 */

tu_clear_screen()

{
	if (clear_screen)
	{
		tputs(clear_screen, 1, tc_putchar);
		return 0;
	}
	/* later, use clr_eos and clr_eol to clear the screen */
	return 1;
}

/* = = = = = = = = = = = = = tu_cup = = = = = = = = = = = = = = =*/
/*
 * Find some way to clear the terminal screen.
 */

tu_cup(row, col)

int	row;
int	col;

{
	if (cursor_address)
	{
		tputs(tparm(cursor_address, row, col), 1, tc_putchar);
		return 0;
	}
	return 1;
}

