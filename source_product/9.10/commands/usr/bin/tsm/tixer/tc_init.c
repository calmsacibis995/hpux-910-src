/*****************************************************************************
** Copyright (c)        1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: tc_init.c,v 70.1 92/03/09 16:13:23 ssa Exp $ */
/*
 * File: tc_init.c
 * Creator: G.Clark.Brown
 *
 * Test routines for simple cursor movement (init, cr, etc.)in the Terminfo
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

#include	<sys/types.h>
#include	<stdio.h>
#include	<curses.h>
#include	<term.h>
#include	"tixermacro.h"
#include	"tixer.h"
#include	"tixerext.h"

# include <sys/stat.h>

# define exists(file)		(stat(file,&Statbuf)<0 ? 0:Statbuf.st_mode)

/* = = = = = = = = = = = = = = = = tc_init = = = = = = = = = = = = = = = = =*/
/*
 * Test init capabilities.
 */
tc_init(p_opts)

struct option_tab	*p_opts;
{
	tu_clear_screen();
	print_cap_title(p_opts);
	u_print_command("tput init");

	return 0;
}

/* = = = = = = = = = = = = = = = = tc_reset = = = = = = = = = = = = = = = = =*/
/*
 * Test reset capabilities.
 */
tc_reset(p_opts)

struct option_tab	*p_opts;
{
	if(exists("/bin/reset"))
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		u_print_command("tput reset");
		return 0;
	}
	return 1;
}
