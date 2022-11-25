/*****************************************************************************
** Copyright (c) 1990        Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: rstorage.c,v 70.1 92/03/09 16:41:06 ssa Exp $ */
/**************************************************************************
* rstorage.c
*	Localize storage functions to minimize lint complaint and
*	centralize error messages. receiver only.
**************************************************************************/
#include <sys/types.h>
#include <malloc.h>

/**************************************************************************
* malloc_run
*	Allocate "bytes" bytes, using "operation" as an error message.
**************************************************************************/
long *
malloc_run( bytes, operation )
	int	bytes;
	char	*operation;
{
	long	*ptr;
	char	buff[ 80 ];

	ptr = (long *) malloc( bytes );
	return( ptr );
}
#ifdef lint
static int lint_alignment_warning_ok_1;
#endif
