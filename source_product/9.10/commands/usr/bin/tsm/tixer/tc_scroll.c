/*****************************************************************************
** Copyright (c)        1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: tc_scroll.c,v 70.1 92/03/09 16:13:33 ssa Exp $ */
/*
 * File: tc_clear.c
 * Creator: G.Clark.Brown
 *
 * Test routines for area clears in the Terminfo Exerciser Program.
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

static char		garbage_string[]="GARBAGE-";

static char	*testpattern[]= {
					"When he returned home late that",
					"night, he noticed",
"that the front door to his apartment was unlocked.  He went in cautiously",
					"and discovered",
"his belongings thrown about the room.  It was as though",
					"they had been looking for",
"something.  But what?  He didn't have anything that",
					"would be",
					""
				};

#ifdef change_scroll_region

/* = = = = = = = = = = = = = = = = tc_csr = = = = = = = = = = = = = = = = =*/
/*
 * Test change_scroll_region capability.
 */
tc_csr(p_opts)

struct option_tab	*p_opts;
{
	int	row, col;

	if(change_scroll_region)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		fputs(
		    "\r\n****** This line is above the scrolling region ******",
		    stdout);
		for(row=4; row<lines-5; row++)
		{
			tu_cup(row, 0);
			for (col=0; col<columns-1; col++)
				tc_putchar(garbage_string[
					col%(sizeof(garbage_string)-1)
						 	 ]);
		}
		tu_cup(lines-4, 0);
		fputs("****** This line is below the scrolling region ******",
			stdout);
		tputs(tparm(change_scroll_region, 4, lines-5), 1, tc_putchar);
		tu_cup(lines-5, 0);
		for(row=3; row<lines-5; row++)
		{
			fputs("\r\n", stdout);
			printf(testpattern[
				(row-3)%(sizeof(testpattern)/sizeof(char *))
						 	 ]);
		}
		tputs(tparm(change_scroll_region, 0, lines-1), 1, tc_putchar);
		tu_cup(lines-2, 0);

		return 0;
	}
	return 1;
}

#endif

#ifdef scroll_forward

/* = = = = = = = = = = = = = = = = tc_ind = = = = = = = = = = = = = = = = =*/
/*
 * Test scroll_forward capability.
 */
tc_ind(p_opts)

struct option_tab	*p_opts;
{
	int	row, col;

	if(scroll_forward)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		for(row=4; row<lines-1; row++)
		{
			tu_cup(row, 0);
			for (col=0; col<columns-1; col++)
				tc_putchar(garbage_string[
					col%(sizeof(garbage_string)-1)
						 	 ]);
		}
		tu_cup(lines-1, 0);
		fputs("* This should be the top line on a nearly blank screen",
			stdout);

		for (row=0; row<lines-1; row++)
		{
			tu_cup(lines-1, 0);
			tputs(scroll_forward, 1, tc_putchar);
			for (col=0; col<columns-1; col++)
				tc_putchar(' ');
		}
		tu_cup(lines-2, 0);

		return 0;
	}
	return 1;
}

#endif

#ifdef scroll_reverse

/* = = = = = = = = = = = = = = = = tc_ri = = = = = = = = = = = = = = = = =*/
/*
 * Test scroll_reverse capability.
 */
tc_ri(p_opts)

struct option_tab	*p_opts;
{
	int	row, col;

	if(scroll_reverse)
	{
		tu_clear_screen();
		fputs(
"* This should be the bottom line on a nearly blank screen.\r\n",
			stdout);
		print_cap_title(p_opts);
		for(row=4; row<lines-1; row++)
		{
			tu_cup(row, 0);
			for (col=0; col<columns-1; col++)
				tc_putchar(garbage_string[
					col%(sizeof(garbage_string)-1)
						 	 ]);
		}
		for (row=0; row<lines-1; row++)
		{
			tu_cup(0, 0);
			tputs(scroll_reverse, 1, tc_putchar);
			for (col=0; col<columns-1; col++)
				tc_putchar(' ');
		}
		tu_cup(lines-5, 0);

		return 0;
	}
	return 1;
}

#endif

#ifdef parm_index

/* = = = = = = = = = = = = = = = = tc_indn = = = = = = = = = = = = = = = = =*/
/*
 * Test parm_index capability.
 */
tc_indn(p_opts)

struct option_tab	*p_opts;
{
	int	row, col;

	if(parm_index)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		for(row=4; row<lines-1; row++)
		{
			tu_cup(row, 0);
			for (col=0; col<columns-1; col++)
				tc_putchar(garbage_string[
					col%(sizeof(garbage_string)-1)
						 	 ]);
		}
		tu_cup(lines-1, 0);
		fputs("* This should be the top line on a nearly blank screen",
			stdout);

		tu_cup(lines-1, 0);
			tputs(tparm(parm_index, lines-1), 1, tc_putchar);
		tu_cup(lines-2, 0);

		return 0;
	}
	return 1;
}

#endif

#ifdef parm_rindex

/* = = = = = = = = = = = = = = = = tc_rin = = = = = = = = = = = = = = = = =*/
/*
 * Test parm_rindex capability.
 */
tc_rin(p_opts)

struct option_tab	*p_opts;
{
	int	row, col;

	if(parm_rindex)
	{
		tu_clear_screen();
		fputs(
"* This should be the bottom line on a nearly blank screen.\r\n",
			stdout);
		print_cap_title(p_opts);
		for(row=4; row<lines-1; row++)
		{
			tu_cup(row, 0);
			for (col=0; col<columns-1; col++)
				tc_putchar(garbage_string[
					col%(sizeof(garbage_string)-1)
						 	 ]);
		}
		tu_cup(0, 0);
		for (row=0; row<lines-1; row++)
			tputs(tparm(parm_rindex, lines-1), 1, tc_putchar);
		tu_cup(lines-5, 0);

		return 0;
	}
	return 1;
}

#endif

#ifdef memory_lock
#ifdef memory_unlock

/* = = = = = = = = = = = = = = = = tc_meml = = = = = = = = = = = = = = = = =*/
/*
 * Test memory_lock capability.
 */
tc_meml(p_opts)

struct option_tab	*p_opts;
{
	int	row, col;

	if(memory_lock)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		fputs(
		    "\r\n****** This line is above the scrolling region ******",
		    stdout);
		for(row=4; row<lines-5; row++)
		{
			tu_cup(row, 0);
			for (col=0; col<columns-1; col++)
				tc_putchar(garbage_string[
					col%(sizeof(garbage_string)-1)
						 	 ]);
		}
		tu_cup(4, 0);
		tputs(memory_lock, 1, tc_putchar);
		tu_cup(lines-1, 0);
		for(row=4; row<lines-4; row++)
		{
			fputs("\r\n", stdout);
			printf(testpattern[
				(row-3)%(sizeof(testpattern)/sizeof(char *))
						 	 ]);
		}
		for(row=0; row<3; row++)
			fputs("\r\n", stdout);
		tputs(memory_unlock, 1, tc_putchar);
		tu_cup(lines-2, 0);

		return 0;
	}
	return 1;
}

#endif
#endif
