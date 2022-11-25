/*****************************************************************************
** Copyright (c) 1990 Structured Software Solutions, Inc.                   **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: testmprog.c,v 70.1 92/03/09 16:16:23 ssa Exp $ */
#include <stdio.h>
#include <curses.h>
#include <term.h>
#include "fmenu.h"

char String[] = "This is another program";
extern char *getenv();

main(argc, argv)
int argc;
char *argv[];
{
	int row, col, ncols;
	char x;
	int i;
	char *envstring;

	term_init();
	envstring = getenv( "MENU_ROW" );
	row = atoi( envstring );
	envstring = getenv( "MENU_COL" );
	col = atoi( envstring );
	ncols = strlen( String ) + 6;

	if ( Fm_magic_cookie || Fm_ceol_standout_glitch )
	{
		for( i = 0; i < 3; i++ )
		{
			if ( Fm_ceol_standout_glitch )
			{
				term_cursor_address( row + i, col );
				term_clear_line();
			}
			else
			{
				term_cursor_address( row + i, col + ncols - 1 );
				term_write( "%a%n", FM_ATTR_NONE );
			}
		}
	}
	term_cursor_address( row, col );
	term_write( "%a %a%g1%c%r%c%g0%a %n", FM_ATTR_SHADOW, FM_ATTR_BOX,
		    UPPER_LEFT, HORIZONTAL, ncols - 4, UPPER_RIGHT,
		    FM_ATTR_SHADOW);
	term_cursor_address( row+1, col );
	term_write( "%a %a%g1%c%g0%a%b%s%b%a%g1%c%g0%a %n",
		    FM_ATTR_SHADOW, FM_ATTR_BOX, VERTICAL,
		    FM_ATTR_TITLE, 1, String,
		    1, FM_ATTR_BOX, VERTICAL, FM_ATTR_SHADOW );
	term_cursor_address( row+2, col );
	term_write( "%a %a%g1%c%r%c%g0%a ", FM_ATTR_SHADOW, FM_ATTR_BOX,
		    LOWER_LEFT, HORIZONTAL, ncols - 4, LOWER_RIGHT,
		    FM_ATTR_SHADOW);
	read( 0, &x, 1 );
}
