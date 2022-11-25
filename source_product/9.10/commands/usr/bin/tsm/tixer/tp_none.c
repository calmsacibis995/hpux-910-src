/*****************************************************************************
** Copyright (c)        1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: tp_none.c,v 70.1 92/03/09 16:14:26 ssa Exp $ */
/*
 * File:		tp_none.c
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

/* = = = = = = = = = = = = = = = tp_none = = = = = = = = = = = = = = = =*/
/*
 * This is the "Not yet implemented" entry for the cap table.  It is called
 * for capabilities that we have not written a real test for.
 */

tp_none(p_opts)

struct option_tab	*p_opts;

{

	print_cap_title(p_opts);
	printf("    Test not implemented.\r\n");
}

/* = = = = = = = = = = = = = = = tp_key_none = = = = = = = = = = = = = = = =*/
/*
 * This prints a message that there are no tests for keys.  It is called
 * for capabilities that describe the string sent by keys.
 */

tp_key_none(p_opts)

struct option_tab	*p_opts;

{

	print_cap_title(p_opts);
	printf("    No tests are implemented for 'key' capabilities.\r\n");
}
