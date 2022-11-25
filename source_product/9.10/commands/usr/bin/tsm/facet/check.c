/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: check.c,v 70.1 92/03/09 15:39:29 ssa Exp $ */
#define FACET_PROGRAM 1
wait_return_pressed()
{
	char	buff[ 80 ];

	printf( "Press RETURN to exit.\r\n" );
	gets( buff );
}
wait_return_pressed_continue()
{
	char	buff[ 80 ];

	printf( "Press RETURN to continue.\r\n" );
	gets( buff );
}
