/*****************************************************************************
** Copyright (c) 1986 - 1989 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: startutil.c,v 70.1 92/03/09 16:40:15 ssa Exp $ */
/* #define DEBUG 1 */

#include	"ptydefs.h"

#include	<stdio.h>
#include	<sys/types.h>
#include	<fcntl.h>

#include	<termio.h>

#include	<ctype.h>
#include	"facetterm.h"

#include	<errno.h>
extern int	errno;

set_window_stty()
{
	return;
}

/******************************************************************
* Some ptys need line discipline pushed - if so do it here.
******************************************************************/
set_window_line_discipline()
{
	return( 0 );
}
					/* no PUSH_PTEM_LDTERM_ON_OPEN */
