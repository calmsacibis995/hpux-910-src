/*****************************************************************************
** Copyright (c)        1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: tc_status.c,v 70.1 92/03/09 16:13:42 ssa Exp $ */
/*
 * File: tc_status.c
 * Creator: G.Clark.Brown
 *
 * Test routines for status line in the Terminfo Exerciser Program.
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


#ifdef to_status_line
#ifdef from_status_line

/* = = = = = = = = = = = = = = = = tc_tsl = = = = = = = = = = = = = = = = =*/
/*
 * Test to_status_line capability.
 */
tc_tsl(p_opts)

struct option_tab	*p_opts;
{
	char	buffer[132];

	if(to_status_line && from_status_line)
	{
		strcpy(buffer,
			"STATUS LINE - this should be on the status line.");
#ifdef width_status_line
		if(width_status_line >0 && width_status_line < sizeof(buffer))
			buffer[width_status_line-2]='\0';
#endif
		tu_clear_screen();
		print_cap_title(p_opts);
		printf(
	   "This line should be on the non-status part of the screen.\r\n");
		tputs(tparm(to_status_line, 0), 1, tc_putchar);
		printf(buffer);
		tputs(from_status_line, 1, tc_putchar);
		printf(
	   "This line should also be on the non-status part of the screen.\r\n");
		printf(
"There should be a message on the status line that starts with \"STATUS LINE\".\r\n");
#ifdef disable_status_line
		if(disable_status_line)
		{
			tu_pranykey(p_opts);
			tputs(disable_status_line, 1, tc_putchar);
			printf(
	   "Now the message on the status line should be gone.\r\n");
		}
#endif
		return 0;
	}
	return 1;
}

#endif
#endif
