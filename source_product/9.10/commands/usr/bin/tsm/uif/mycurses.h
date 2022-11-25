/*****************************************************************************
** Copyright (c) 1990 Structured Software Solutions, Inc.                   **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: mycurses.h,v 70.1 92/03/09 16:56:36 ssa Exp $ */
#ifdef XENIX386
#include	<tinfo.h>
#else

#ifndef	NO_INCLUDE_TERMIO_H
#include	<termio.h>
#endif

#include	<curses.h>

#endif
