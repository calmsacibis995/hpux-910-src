/*****************************************************************************
** Copyright (c) 1990        Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: person.c,v 66.6 90/10/18 09:26:00 kb Exp $ */
#include "person.h"
int	M_pe = 0;		/* personality of terminal */
int	X_pe = 0;		/* personality of being read from .fi file */
int	Install_all_personalities = 0;
char *dec_encode();
char *temp_encode();
/**************************************************************************
* extra_personality
*	TERMINAL DESCRIPTION PARSER module for 'NEW_PERSONALITY'
*	and 'INSTALL_ALL_PERSONALITIES' for HP ansi mode.
**************************************************************************/
/*ARGSUSED*/
extra_personality( buff, string, attr_on_string, attr_off_string ) 
	char	*buff;
	char	*string;
	char	*attr_on_string;
	char	*attr_off_string;
{

	if ( strcmp( buff, "NEW_PERSONALITY" ) == 0 )
	{
		if ( X_pe < MAX_PERSONALITY - 1 )
		{
			X_pe++;
		}
		else
		{
			printf( "Too many personality\n" );
		}
	}
	else if ( strcmp( buff, "INSTALL_ALL_PERSONALITIES" ) == 0 )
	{
		Install_all_personalities = 1;
	}
	else
	{
		return( 0 );		/* no match */
	}
	return( 1 );
}
