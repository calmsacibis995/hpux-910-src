/*****************************************************************************
** Copyright (c)        1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: tp_all.c,v 70.1 92/03/09 16:14:21 ssa Exp $ */
/*
 * File:		tp_all.c
 * Creator:		G.Clark.Brown
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
extern		tp_none();
extern		tp_key_none();


/* = = = = = = = = = = = = = = = tp_all = = = = = = = = = = = = = = = =*/
/*
 * Test all single terminfo capabilities for a terminal.
 */

tp_all(p_opts)

struct option_tab	*p_opts;

{
	int	(*test_routine)();
	int	ret;
	int	i;
/*
 * First, duplicate the routines into the dup_routine field.
 */
	for (i=0; cap_table[i].variable_name; i++)
		if (cap_table[i].routine != tp_none &&
			cap_table[i].routine != tp_key_none)
				cap_table[i].dup_routine=cap_table[i].routine;
/*
 * Loop through the table, call every routine that has been defined for this
 * machine.
 */
	for (p_opts->captab=0; cap_table[p_opts->captab].variable_name;
		p_opts->captab++)
	{
		if(test_routine=cap_table[p_opts->captab].dup_routine)
		{
/*
 * After each routine is called once, find any other copies in dup_routine
 * and prevent them from being called.
 */
			for (i=p_opts->captab; cap_table[i].variable_name; i++)
				if (cap_table[i].dup_routine==test_routine)
					cap_table[i].dup_routine=NULL;
			do
			{
				if((ret=(*test_routine)(p_opts))<0)
					fprintf(stderr, "     Error in test.");
				fflush(stdout);
			}
			while(!ret && !continue_prompt(p_opts));

		}
	}
	return 0;
}
