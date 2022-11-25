/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: paste.h,v 66.3 90/09/20 12:15:55 kb Exp $ */
#define MAX_PASTE_EOL	10
extern int	T_paste_eol_no;
extern char	*T_paste_eol[ MAX_PASTE_EOL ];
#define MAX_PASTE_EOL_NAME_LEN	20
extern char	T_paste_eol_name[ MAX_PASTE_EOL ][ MAX_PASTE_EOL_NAME_LEN + 1 ];
