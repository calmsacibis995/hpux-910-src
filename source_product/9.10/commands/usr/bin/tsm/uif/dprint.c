/*****************************************************************************
** Copyright (c) 1990 Structured Software Solutions, Inc.                   **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: dprint.c,v 70.1 92/03/09 16:15:25 ssa Exp $ */
/*
 * dprint
 *
 * Copyright (c) Structured Software Solutions, Inc 1989. All rights reserved.
 */

#include	"facetterm.h"

extern char *ttyname();

int dbgfd;

open_debug()
{
	char buffer[ FIOC_BUFFER_SIZE ];

	strcpy( buffer, "ptyname_9" );
	if ( fct_ioctl( 0, FIOC_GET_INFORMATION, buffer ) == -1 )
		return( -1 );
	dbgfd = open( buffer, 1 );
	return( dbgfd );
}


dprint (fmt, args)
char *fmt, *args;
{
	char **s_argptr, *fmtptr;
	int *i_argptr;
	char buff[16];
	char *strptr;
	int len;

	s_argptr = &args;
	i_argptr = (int *)(&args);
	for (fmtptr = fmt; *fmtptr; fmtptr++)
	{
		if (*fmtptr == '%')
		{
			fmtptr++;
			if (*fmtptr == 's')
			{
				strptr = *s_argptr;
				len = strlen (strptr);
				s_argptr++;
				i_argptr = (int *)(s_argptr);
			}
			else if (*fmtptr == 'c')
			{
				strptr = buff;
				buff[0] = *i_argptr;
				len = 1;
				i_argptr++;
				s_argptr = (char **)(i_argptr);
			}
			else if (*fmtptr == 'x')
			{
				strptr = buff;
				len = itos ( (unsigned) *i_argptr, buff, 16);
				i_argptr++;
				s_argptr = (char **)(i_argptr);
			}
			else if (*fmtptr == 'd')
			{
				strptr = buff;
				len = itos ( (unsigned) *i_argptr, buff, 10);
				i_argptr++;
				s_argptr = (char **)(i_argptr);
			}
			else if (*fmtptr == 'o')
			{
				strptr = buff;
				len = itos ( (unsigned) *i_argptr, buff, 8);
				i_argptr++;
				s_argptr = (char **)(i_argptr);
			}
			else
			{
				strptr = fmtptr - 1;
				len = 2;
			}
		}
		else
		{
			strptr = fmtptr;
			len = 1;
		}
		while (len--)
		{
			write( dbgfd, strptr, 1 );
			strptr++;
		}
	}
}
