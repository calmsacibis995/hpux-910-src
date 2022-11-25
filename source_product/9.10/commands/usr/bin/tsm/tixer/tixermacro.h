/*****************************************************************************
** Copyright (c)        1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: tixermacro.h,v 70.1 92/03/09 16:14:17 ssa Exp $ */
/*
 * File:		tixermacro.h
 * Creator:		G.Clark.Brown
 * Creation Date:	10/7/88
 *
 * Constants and macro definitions for the Terminfo Exerciser Program.
 *
 * Module name: 	%M%
 * SID:			%I%
 * Extract date:	%H% %T%
 * Delta date:		%G% %U%
 * Stored at:		%P%
 * Module type:		%Y%
 * 
 */

#ifdef lint
#undef NULL
#define NULL	(0)
#endif

#define		DEBUG	(1)

#define		ERR0(string)	(fprintf(stderr, string))
#define		ERR1(string, arg1)	(fprintf(stderr, string, arg1))
#define		ERR2(string, arg1, arg2) (fprintf(stderr, string, arg1, arg2))

#define		U_strcpy(s1, s2) (strncpy(s1, s2, sizeof(s1)), s1[sizeof(s1)-1] = '\0')
#define		U_strcat(s1, s2) (strncat(s1, s2, sizeof(s1)-1))

#define		OPT	int
#define		OPT3	int

#ifndef		TRUE
#define		TRUE	(-1)
#endif

#ifndef		FALSE
#define		FALSE	(0)
#endif

/*
 * Message Levels
 */

#define U_TERMINATE	(0)
#define U_ERROR		(1)
#define U_WARNING	(2)
#define U_MESSAGE	(3)
#define U_NOMESSAGE	(4)
#define DETAIL		(2)

#define SUMMARY		(1)

#define TIXC_BOOL	(0)
#define TIXC_NUMBER	(1)
#define TIXC_STRING	(2)

#define TIXC_MAXCAPS	(500)

