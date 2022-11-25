/*****************************************************************************
** Copyright (c)        1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: tc_clear.c,v 70.1 92/03/09 16:12:59 ssa Exp $ */
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

#ifdef clr_eol

/* = = = = = = = = = = = = = = = = tc_el = = = = = = = = = = = = = = = = =*/
/*
 * Test clr_eol capability.
 */
tc_el(p_opts)

struct option_tab	*p_opts;
{
	int	row, col;

	if(clr_eol)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		for(row=3; row<lines-5; row++)
		{
			tu_cup(row, 0);
			for (col=0; col<columns-1; col++)
				tc_putchar(garbage_string[
					col%(sizeof(garbage_string)-1)
						 	 ]);
		}
		for(row=3; row<lines-5; row++)
		{
			tu_cup(row, 0);
			printf(testpattern[
				(row-3)%(sizeof(testpattern)/sizeof(char *))
						 	 ]);
			tputs(clr_eol, 1, tc_putchar);
			fflush(stdout);
		}
		tu_cup(lines-4, 0);

		return 0;
	}
	return 1;
}

#endif

#ifdef clr_eos

/* = = = = = = = = = = = = = = = = tc_ed = = = = = = = = = = = = = = = = =*/
/*
 * Test clr_eos capability.
 */
tc_ed(p_opts)

struct option_tab	*p_opts;
{
	int	row, col;

	if(clr_eos)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		for(row=10; row<lines-1; row++)
		{
			tu_cup(row, 0);
			for (col=0; col<columns; col++)
				tc_putchar(garbage_string[
					col%(sizeof(garbage_string)-1)
						 	 ]);
		}
		for(row=3; row<10; row++)
		{
			tu_cup(row, 0);
			printf(testpattern[
				(row-3)%(sizeof(testpattern)/sizeof(char *))
						 	 ]);
		}
		tu_cup(row, 0);
		tputs(clr_eos, 1, tc_putchar);
		tu_cup(lines-4, 0);

		return 0;
	}
	return 1;
}

#endif


#ifdef clear_screen

/* = = = = = = = = = = = = = = = = tc_clear = = = = = = = = = = = = = = = = =*/
/*
 * Test clear_screen capability.
 */
tc_clear(p_opts)

struct option_tab	*p_opts;
{
	int	row, col;

	if(clear_screen)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		for(row=3; row<lines; row++)
		{
			tu_cup(row, 0);
			for (col=0; col<columns; col++)
				tc_putchar(garbage_string[
					col%(sizeof(garbage_string)-1)
						 	 ]);
		}
		tputs(clear_screen, 1, tc_putchar);
		printf(
"*<--- This star should be in the upper left corner of a nearly blank screen.");
		tu_cup(lines-4, 0);

		return 0;
	}
	return 1;
}

#endif

#ifdef erase_chars

/* = = = = = = = = = = = = = = = = tc_ech = = = = = = = = = = = = = = = = =*/
/*
 * Test erase_chars capability.
 */
tc_ech(p_opts)

struct option_tab	*p_opts;
{
	int	row, col;

	if(erase_chars)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		for(row=3; row<lines-5 && row<columns/6+1; row++)
		{
			tu_cup(row, 0);
			for (col=0; col<columns; col++)
				if (col>row*3 && col<columns-row*3)
					tc_putchar(garbage_string[
						col%(sizeof(garbage_string)-1)
						 	 ]);
				else if (col==row*3 || col==columns-row*3)
					tc_putchar('*');
				else
					tc_putchar(' ');
		}
		for(row=3; row<lines-5 && row<columns/6+1; row++)
		{
			tu_cup(row, row*3+1);
			tputs(tparm(erase_chars, columns-row*6-1), 1, tc_putchar);
		}
		tu_cup(lines-4, 0);
		printf(
			"There should be a large V of stars.\r\n");

		return 0;
	}
	return 1;
}

#endif
