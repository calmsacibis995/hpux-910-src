/*****************************************************************************
** Copyright (c)        1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: tp_short.c,v 70.1 92/03/09 16:14:40 ssa Exp $ */
/*
 * File:		tp_short.c
 * Creator:		G.Clark.Brown
 *
 * Short test of Terminfo capabilities.
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
#include	<termio.h>
#include	<errno.h>
#include	<signal.h>

#include	"tixermacro.h"
#include	"tixer.h"
#include	"tixerext.h"

#ifndef lint
	static char	Sccsid[80]="%W% %E% %G%";
#endif

extern		errno;
extern		count_seconds();
extern unsigned	alarm();

static int	seconds;

/* = = = = = = = = = = = = = = = tp_short = = = = = = = = = = = = = = = =*/
/*
 * Test terminfo capabilities for a terminal, but only a few standard
 * attributes and some cursor positioning.  Since these tests use the curses
 * library (which is a higher level than terminfo), they may try to use
 * attributes that do not work on a particular terminal.  This will show
 * at a glance what the terminal's terminfo description is capable of.
 */

tp_short(p_opts)

struct option_tab	*p_opts;

{

	printf("\r\n\n");
	printf("Terminal test using TERM=%s\r\n", p_opts->type);
/*
 * Cursor addressing test.
 */
	printf("\r\n\n\n\n********** Step 1: Simple Cursor Test. **********\r\n\n");
	
	sleep( 2 );
	do
		cursor_test(p_opts);
	while(!continue_prompt(p_opts));
/*
 * Attribute test.
 */
	printf("\r\n\n\n\n********** Step 2: Character Attribute Test. **********\r\n\n");
	sleep( 2 );
	do
		attribute_test(p_opts);
	while(!continue_prompt(p_opts));
}


/* = = = = = = = = = = = = = = = cursor_test = = = = = = = = = = = = = = = =*/
/*
 * Cursor control and addressing test.
 */

cursor_test(p_opts)

struct option_tab	*p_opts;

{

	term_cursor_pattern();
	return 0;
}

/* = = = = = = = = = = = = = = = attribute_test = = = = = = = = = = = = = = = =*/
/*
 * Test pattern to show blink, reverse, standout, etc.
 */

attribute_test(p_opts)

struct option_tab	*p_opts;

{


	term_attrib_pattern();
	return 0;
}
