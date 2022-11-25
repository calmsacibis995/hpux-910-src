/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: textkey.c,v 66.6 90/10/18 09:27:46 kb Exp $ */
/**************************************************************************
* textkey.c
*	Allow override of the text strings presented to the user - receiver.
**************************************************************************/
#include "foreign.h"

extern char	*Text_receiver_terminating;

char	*Text_end_of_list = "";

TEXTNAMES Textnames[] =
{
/* commonkey.c */
	"receiver_terminating",		&Text_receiver_terminating,

/* end of list */
	"",				&Text_end_of_list
};
