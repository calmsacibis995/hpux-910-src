/*****************************************************************************
** Copyright (c)        1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: tc_tab.c,v 70.1 92/03/09 16:13:47 ssa Exp $ */
/*
 * File: tc_tab.c
 * Creator: G.Clark.Brown
 *
 * Test routines for tabs in the Terminfo Exerciser Program.
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

#ifdef tab

/* = = = = = = = = = = = = = = = = tc_ht = = = = = = = = = = = = = = = = =*/
/*
 * Test tab capability.
 */
tc_ht(p_opts)

struct option_tab	*p_opts;
{
	int	row, col;

	if(tab)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		for (col=0; col<columns/8-1; col++)
		{
			tputs(tab, 1, tc_putchar);
			printf("Tab_%d", col+1);
		}
		for(row=5; row<lines-2 && row-4<columns/8; row++)
		{
			tu_cup(row, 0);
			for (col=0; col<row-4; col++)
				tputs(tab, 1, tc_putchar);
			printf("Tab_%d", row-4);
		}
		tu_cup(lines-2, 0);

		return 0;
	}
	return 1;
}

#endif

#ifdef back_tab

/* = = = = = = = = = = = = = = = = tc_bt = = = = = = = = = = = = = = = = =*/
/*
 * Test back_tab capability.
 */
tc_bt(p_opts)

struct option_tab	*p_opts;
{
	int	row, col;
	if(back_tab)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		tu_cup(3, columns-1);
		for (col=0; col<columns/8; col++)
		{
			tputs(back_tab, 1, tc_putchar);
			printf("Back_%d", col+1);
			tputs(back_tab, 1, tc_putchar);
		}
		tu_cup(lines-2, 0);

		return 0;
	}
	return 1;
}

#endif


#ifdef init_tabs

/* = = = = = = = = = = = = = = = = tc_it = = = = = = = = = = = = = = = = =*/
/*
 * Test init_tabs capability.
 */
tc_it(p_opts)

struct option_tab	*p_opts;
{
	int	row, col;
	if(init_tabs>0)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		printf("init_tabs is set to %d\r\n", init_tabs);
		if (init_tabs==8)
			printf("This is the normal value.\r\n");
		else
		{
			printf(
"This is an unusual value that will cause many applications (including\r\n");
			printf(
		"the tab tests in this program) to fail.\r\n");	
		}

		return 0;
	}
	return 1;
}

#endif

