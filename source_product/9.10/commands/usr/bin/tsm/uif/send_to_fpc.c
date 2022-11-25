/*****************************************************************************
** Copyright (c) 1990 Structured Software Solutions, Inc.                   **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: send_to_fpc.c,v 70.1 92/03/09 16:16:13 ssa Exp $ */
/*
 * send_to_fpc.c
 *
 * Facet/PC aware calls for the Facet User Interface.
 *
 * Copyright (c) Structured Software Solutions, Inc 1989. All rights reserved.
 *
 */

#include	"facetterm.h"
#include	"facetpc.h"

fpc_init( menu_hot_key_char, notify_when_current_char )
	int	menu_hot_key_char;
	int	notify_when_current_char;
{
	printf( "\033[%d;%d;%dF", POPUP_CTRL_WINDOW, menu_hot_key_char,
		notify_when_current_char );
	term_outgo();
}


fpc_send_select_window( window )
int window;
{
	printf( "\033[%d;%dF", SELECT_WIN, window );
	term_outgo();
}


fpc_pop_down()
{
}


fpc_quit()
{
	printf( "\033[%dF", POPUP_CTRL_END );
	term_outgo();
}
