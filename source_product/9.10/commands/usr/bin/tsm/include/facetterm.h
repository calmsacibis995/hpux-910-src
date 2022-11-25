/*****************************************************************************
** Copyright (c) 1986 - 1989 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: facetterm.h,v 70.1 92/03/09 15:48:00 ssa Exp $ */
/*
 * facetterm.h     %W% %U% %G%
 *
 * Header for user processes wishing to communicate with facetterm
 * via special window ioctl calls.
 */


#ifndef FACETTERM_H

#define FACETTERM_H

/************************************************************************/
/* window system call functions						*/
/************************************************************************/

#define FIOC_BUFFER_SIZE	48

						/* ======== HP ============== */
#include	<sys/ioctl.h>

				/* indicates window system call to ioctl */
#define FIOC	F		
typedef char Fioc_buffer[ FIOC_BUFFER_SIZE ];

#if ( ! defined( HPREV ) ) || ( HPREV != 6 )
						/* ======== REV 7 ETC ======= */
						/* report active windows */
#define FIOC_ACTIVE		_IOR('F', 'A', Fioc_buffer )
						/* report idle windows */
#define FIOC_IDLE		_IOR('F', 'I', Fioc_buffer )
						/* do window mode commands */
#define FIOC_WINDOW_MODE	_IOW('F', 'W', Fioc_buffer )
						/* report current windows */
#define FIOC_CURWIN		_IOR('F', 'C', Fioc_buffer )
#define FIOC_PROFILE_ON		_IO('F', 'X', int )	/* profile on */
#define FIOC_PROFILE_OFF	_IO('F', 'Y', int )	/* profile off */
						/* name of program on window */
#define FIOC_GET_PROGRAM_NAME	_IOWR('F', 'N', Fioc_buffer )
						/* push popup screen*/
#define FIOC_PUSH_POPUP_SCREEN	_IOR('F', 'P', Fioc_buffer )	
						/* pop popup screen*/
#define FIOC_POP_POPUP_SCREEN	_IOR('F', 'O', Fioc_buffer )	
						/* ttynames etc. */
#define FIOC_GET_INFORMATION	_IOWR('F', 'G', Fioc_buffer )	
						/* paste into window */
#define FIOC_PASTE		_IOW('F', 'p', Fioc_buffer )	
						/* make utmp entry */
#define FIOC_UTMP		_IOW('F', 'u', Fioc_buffer )

						/* ======== REV 7 ETC ======= */
#else
						/* ======== REV 6 =========== */
						/* report active windows */
#define FIOC_ACTIVE		_IOR(F, 'A', Fioc_buffer )
						/* report idle windows */
#define FIOC_IDLE		_IOR(F, 'I', Fioc_buffer )
						/* do window mode commands */
#define FIOC_WINDOW_MODE	_IOW(F, 'W', Fioc_buffer )
						/* report current windows */
#define FIOC_CURWIN		_IOR(F, 'C', Fioc_buffer )
#define FIOC_PROFILE_ON		_IO(F, 'X', int )	/* profile on */
#define FIOC_PROFILE_OFF	_IO(F, 'Y', int )	/* profile off */
						/* name of program on window */
#define FIOC_GET_PROGRAM_NAME	_IOWR(F, 'N', Fioc_buffer )
						/* push popup screen*/
#define FIOC_PUSH_POPUP_SCREEN	_IOR(F, 'P', Fioc_buffer )	
						/* pop popup screen*/
#define FIOC_POP_POPUP_SCREEN	_IOR(F, 'O', Fioc_buffer )	
						/* ttynames etc. */
#define FIOC_GET_INFORMATION	_IOWR(F, 'G', Fioc_buffer )	
						/* paste into window */
#define FIOC_PASTE		_IOW(F, 'p', Fioc_buffer )	
						/* make utmp entry */
#define FIOC_UTMP		_IOW(F, 'u', Fioc_buffer )	
						/* ======== REV 6 =========== */
#endif
						/* ======== HP ============== */

#define WINDOWS_WINDOW_CHAR_START	'\032'
#define WINDOWS_WINDOW_CHAR_STOP	'\002'
#define WINDOW_MODE_TO_USER_CHAR	'\003'
#define NOTIFY_WHEN_CURRENT_CHAR	'\031'

#endif
