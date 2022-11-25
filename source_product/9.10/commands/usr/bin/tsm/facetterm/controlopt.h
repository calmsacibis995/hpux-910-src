/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: controlopt.h,v 66.3 90/09/20 12:06:40 kb Exp $ */
extern int	Idle_window;
extern int	Idle_window_no_cancel;
#define IDLE_WINDOW_PASTE_CHARS_MAX	20
extern char	Idle_window_paste_chars[];
extern int	Windows_window;
extern int	Windows_window_no_cancel;

extern int	Switch_window;
#define NO_IDLE_WINDOW			-1
#define IDLE_WINDOW_IS_NEXT_ACTIVE	-2
