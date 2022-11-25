/*****************************************************************************
** Copyright (c)        1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: tu_cup.c,v 70.1 92/03/09 16:14:44 ssa Exp $ */
/*
 * File:		tu_cup.c
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
#include	<curses.h>
#include	<term.h>
#include	<errno.h>

#include	"tixermacro.h"
#include	"tixer.h"
#include	"tixerext.h"

#ifndef lint
	static char	Sccsid[80]="%W% %E% %G%";
#endif

extern		errno;

/* = = = = = = = = = = = = = = = has_cup = = = = = = = = = = = = = = = =*/
/*
 * Tests for presence of the required capabilities for doing screen based
 * tests.  The terminfo description must have absolute cursor position
 * and some way to clear the screen.
 */

has_cup(p_opts)

struct option_tab	*p_opts;

{


	if(!cursor_address)		/* no cup */
		return 0;
	if(!clear_screen && !clr_eol && !clr_eos)
		return 0;

	return 1;
}
