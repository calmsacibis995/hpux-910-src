/*****************************************************************************
** Copyright (c)        1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: tc_cursor.c,v 70.1 92/03/09 16:13:13 ssa Exp $ */
/*
 * File: tc_cursor.c
 * Creator: G.Clark.Brown
 *
 * Test routines for cursors in the Terminfo Exerciser Program.
 *
 * Module name: 	%M%
 * SID:			%I%
 * Extract date:	%H% %T%
 * Delta date:		%G% %U%
 * Stored at:		%P%
 * Module type:		%Y%
 * 
 */

#include	<stdio.h>
#include	<curses.h>
#include	<term.h>
#include	"tixermacro.h"
#include	"tixer.h"
#include	"tixerext.h"


#ifdef cursor_normal

/* = = = = = = = = = = = = = = = = tc_cnorm = = = = = = = = = = = = = = = = =*/
/*
 * Test cursor_normal capability.
 */
tc_cnorm(p_opts)

struct option_tab	*p_opts;
{
	int	row, col;
	if(cursor_normal)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		if (cursor_invisible)
		{
			tu_cup(3, 0);
			tputs(cursor_invisible, 1, tc_putchar);
			printf("Cursor should be invisible.");
			tu_cup(4, 0);
			tu_pranykey(p_opts);
			tu_cup(5, 0);
			tputs(cursor_normal, 1, tc_putchar);
			printf("Cursor should be normal.");
			tu_cup(6, 0);
			tu_pranykey(p_opts);
		}
		if (cursor_visible)
		{
			tu_cup(3, 0);
			tputs(cursor_visible, 1, tc_putchar);
			printf("Cursor should be very visible.");
			tu_cup(4, 0);
			tu_pranykey(p_opts);
			tu_cup(5, 0);
			tputs(cursor_normal, 1, tc_putchar);
			printf("Cursor should be normal.");
			tu_cup(6, 0);
			tu_pranykey(p_opts);
		}
		tu_cup(lines-2, 0);

		return 0;
	}
	return 1;
}

#endif
