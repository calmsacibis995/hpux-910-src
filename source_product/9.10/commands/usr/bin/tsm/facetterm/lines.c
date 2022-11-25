/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: lines.c,v 66.6 90/10/18 09:24:20 kb Exp $ */
/**************************************************************************
* lines.c
*	Support modules for terminals changing number of visible rows.
**************************************************************************/
#include "ftwindow.h"
#include "ftterm.h"

/**************************************************************************
* change_display_rows
*	Update window "Outwin" to have "display_rows" number of rows.
**************************************************************************/
/*ARGSUSED*/
change_display_rows( display_rows, has_status_line, does_clear_screen )
	int	display_rows;
	int	has_status_line;
	int	does_clear_screen;
{
	Outwin->display_rows = display_rows;;
	Outwin->display_row_bottom = display_rows - 1;
}
/**************************************************************************
* change_rows_terminal
*	Update the terminal information to show that it has "rows_terminal"
*	number of rows.
*	If this is a change, check for possible impact on split screen
*	configuration.
**************************************************************************/
/*ARGSUSED*/
change_rows_terminal( rows_terminal, has_status_line, does_clear_screen )
	int	rows_terminal;
	int	has_status_line;
	int	does_clear_screen;
{
	if ( rows_terminal != Rows_terminal )
	{
		Rows_terminal = rows_terminal;
		Row_bottom_terminal = rows_terminal - 1;
		adjust_D_split_row( Row_bottom_terminal - 3 );
	}
}
