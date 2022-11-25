/*****************************************************************************
** Copyright (c) 1990 Structured Software Solutions, Inc.                   **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: text.c,v 70.1 92/03/09 16:16:28 ssa Exp $ */
/* #define DEBUG 1 */
#include "foreign.h"

extern char	*Text_must_run_on_window;
extern char	*Text_first_menu_not_found;
extern char	*Text_menu_window_title;
extern char	*Text_window_selection_menu_title;
extern char	*Text_all_windows_except_menu_idle;
char		*Text_end_of_list = "";

TEXTNAMES Textnames[] =
{
/* fmenu.c */
	"must_run_on_window",		&Text_must_run_on_window,
	"first_menu_not_found",		&Text_first_menu_not_found,
	"menu_window_title",		&Text_menu_window_title,
	"window_selection_menu_title",	&Text_window_selection_menu_title,
	"all_windows_except_menu_idle",	&Text_all_windows_except_menu_idle,
/* end of list */
	"",				&Text_end_of_list
};
