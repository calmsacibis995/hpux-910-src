/*****************************************************************************
** Copyright (c)        1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: tc_star.c,v 70.1 92/03/09 16:13:38 ssa Exp $ */
/*
 * File: tc_star.c
 * Creator: G.Clark.Brown
 *
 * Test routines for simple cursor movement (home, cr, etc.)in the Terminfo
 * Exerciser Program.
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

#ifdef cursor_home

/* = = = = = = = = = = = = = = = = tc_home = = = = = = = = = = = = = = = = =*/
/*
 * Test cursor_home capability.
 */
tc_home(p_opts)

struct option_tab	*p_opts;
{
	if(cursor_home)
	{
		tu_clear_screen();
		printf("\r\n\n\n");
		print_cap_title(p_opts);
		tputs(cursor_home, 1, tc_putchar);
		printf(
"* <---------- This star should be in the upper left corner of the screen.");
		tu_cup(12,0);
		return 0;
	}
	return 1;
}
#endif

#ifdef cursor_to_ll

/* = = = = = = = = = = = = = = = = tc_ll = = = = = = = = = = = = = = = = =*/
/*
 * Test cursor_to_ll capability.
 */
tc_ll(p_opts)

struct option_tab	*p_opts;
{
	if(cursor_to_ll)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		tputs(cursor_to_ll, 1, tc_putchar);
		printf(
"* <---------- This star should be in the lower left corner of the screen.");
		tu_cup(12,0);
		return 0;
	}
	return 1;
}
#endif

#ifdef carriage_return

/* = = = = = = = = = = = = = = = = tc_cr = = = = = = = = = = = = = = = = =*/
/*
 * Test carriage_return capability.
 */
tc_cr(p_opts)

struct option_tab	*p_opts;
{
	if(carriage_return)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		printf(
"GARBAGE-GARBAGE-GARBAGE-GARBAGE-GARBAGE-GARBAGE-GARBAGE-GARBAGE-G");
		tputs(carriage_return, 1, tc_putchar);
		printf(
"* <---------- This star should be on the left edge of the screen.\r\n");
		return 0;
	}
	return 1;
}
#endif


#ifdef newline

/* = = = = = = = = = = = = = = = = tc_nel = = = = = = = = = = = = = = = = =*/
/*
 * Test newline capability.
 */
tc_nel(p_opts)

struct option_tab	*p_opts;
{
	if(newline)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		tputs(newline, 1, tc_putchar);
		printf(
"*1<---------- The left edge of the screen should have *1, *2, and *3 on");
		tputs(newline, 1, tc_putchar);
		printf(
"*2<---------- succeeding lines.  (This line should have *2)");
		tputs(newline, 1, tc_putchar);
		printf(
"*3<---------- This line should have *3.");
		tu_cup(12,0);
		return 0;
	}
	return 1;
}
#endif


#ifdef columns

/* = = = = = = = = = = = = = = = = tc_cols = = = = = = = = = = = = = = = = =*/
/*
 * Test columns capability.
 */
tc_cols(p_opts)

struct option_tab	*p_opts;
{
	int	i;

	if(columns)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		printf(
	"The screen should have %d columns, so there should be a star at the\r\n",
			columns);
		printf(
			"right and left edges of the next line.\r\n");
		tc_putchar('*');
		for (i=1; i<columns-1; i++)
			tc_putchar('0'+(i+1)%10);
		tc_putchar('*');
		return 0;
	}
	return 1;
}
#endif


#ifdef lines

/* = = = = = = = = = = = = = = = = tc_lines = = = = = = = = = = = = = = = = =*/
/*
 * Test lines capability.
 */
tc_lines(p_opts)

struct option_tab	*p_opts;
{
	int	i;

	if(lines)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		printf(
	"The screen should have %d lines, so there should be a star in the\r\n",
			lines);
		printf(
			"lower left corner of the screen.\r\n");
		for (i=5; i<lines; i++)
			printf(
				"This should be line %d.\r\n", i);
		printf(
			"* <---             ");
		return 0;
	}
	return 1;
}
#endif


#ifdef eat_newline_glitch

/* = = = = = = = = = = = = = = = = tc_xenl = = = = = = = = = = = = = = = = =*/
/*
 * Test eat_newline_glitch capability.
 */
tc_xenl(p_opts)

struct option_tab	*p_opts;
{
	int	i;

	tu_clear_screen();
	print_cap_title(p_opts);
	if(eat_newline_glitch)
	{
		printf(
"This terminal has the eat_newline_glitch, so there should be a star at the\r\n");
		printf(
"right and left edges of the next two lines with no blank lines between.\r\n");
	}
	else
	{
		printf(
"This terminal does NOT have the eat_newline_glitch. There should be a star\r\n");
		printf(
"at the right and left edges of the next line, a blank line, and another\r\n");
		printf(
"line with stars at the right and left.\r\n");
	}
	tc_putchar('*');
	for (i=1; i<columns-1; i++)
		tc_putchar('0'+(i+1)%10);
	tc_putchar('*');
	tc_putchar('\r');
	tc_putchar('\n');
	tc_putchar('*');
	for (i=1; i<columns-1; i++)
		tc_putchar('0'+(i+1)%10);
	tc_putchar('*');
	return 0;
}
#endif

