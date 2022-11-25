/*****************************************************************************
** Copyright (c)        1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: fdmask.h,v 66.1 90/09/20 13:59:54 kb Exp $ */
		/**********************************************************
		* Composite file descriptor mask of all windows.
		* File descriptor mask indexed by window number 0-9.
		**********************************************************/
extern int	Fd_mask;
extern int	Fd_masks[];
		/**********************************************************
		* File descriptor mask of all blocked windows.
		**********************************************************/
extern int	Fd_blocked_mask;
		/**********************************************************
		* Mask of windows with transparent print chars waiting.
		**********************************************************/
extern int Fd_print_mask;
		/**********************************************************
		* Mask of windows with in printer mode. - These do not show
		* idle even if not open.
		**********************************************************/
extern int Fd_window_printer_mask;
