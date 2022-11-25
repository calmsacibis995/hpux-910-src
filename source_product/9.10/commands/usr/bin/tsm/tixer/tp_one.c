/*****************************************************************************
** Copyright (c)        1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: tp_one.c,v 70.1 92/03/09 16:14:30 ssa Exp $ */
/*
 * File:		tp_one.c
 * Creator:		G.Clark.Brown
 *
 * Test of a single Terminfo capability.
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

/* = = = = = = = = = = = = = = = tp_one = = = = = = = = = = = = = = = =*/
/*
 * Test a single terminfo capability for a terminal.
 */

tp_one(p_opts)

struct option_tab	*p_opts;

{
	int	(*test_routine)();
	int	ret;

	if (!has_cup(p_opts) && cap_table[p_opts->captab].requires_cup)
	{
		print_cap_title(p_opts);
		printf(
"     Cannot be tested because cursor_address and clear are also required.\r\n");
		return 1;
	}

	if (test_routine=cap_table[p_opts->captab].routine)
	{

		if((ret=(*test_routine)(p_opts)) == 1)
		{
			print_cap_title(p_opts);
			printf(
				"     Not defined for this terminal.\r\n");
			return 1;
		}
		if(ret<0)
		{
			print_cap_title(p_opts);
			printf(
				"     Error in test.");
			return -1;
		}
		return 0;
	}
	ERR1("%s capability not supported by terminfo on this machine.\r\n",
			cap_table[p_opts->captab].variable_name);
	return 1;
}
