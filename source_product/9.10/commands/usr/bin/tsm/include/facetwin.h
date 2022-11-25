/*****************************************************************************
** Copyright (c) 1986 - 1989 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: facetwin.h,v 70.1 92/03/09 15:48:05 ssa Exp $ */
/*
 * facetwin.h     %W% %U% %G%
 */


#ifndef WINDOWS_H

#define WINDOWS_H


/************************************************************************/
/* window system parameter definitions					*/
/************************************************************************/

#define USR_WINDS	10		/* number of user accessible windows */
#define	NO_WINDS	USR_WINDS+4+1	/* USR_WINDS + Status line windows +
					   1 menu window */

					/* maximum number of arguments to
					   send in a Facet packet */
#define MAXPROTOARGS	48



/************************************************************************/
/* window system call functions						*/
/************************************************************************/

#define WIOC	('W' << 8)	/* indicates window system call to ioctl */

						/* ========= HP ============= */
#include        <sys/ioctl.h>

extern char Exec_list_buffsize[MAXPROTOARGS ];

#if ( ! defined( HPREV ) ) || ( HPREV != 6 )
						/* ========= REV 7 ETC======= */

				/* notification to PC of first open of
				   a window */
#define FIRST_OPEN  _IOW('W', 53, int) 
				/* notification to PC of last close of
				   a window */
#define LAST_CLOSE  _IOW('W', 54, int) 
				 /* program execution list notification */
#define EXEC_LIST   _IOW('W', 55, Exec_list_buffsize )

				/* program doing a read with O_NDELAY set */
#define NDELAY_READ _IOW('W', 56, int) 

						/* ========= REV 7 ETC======= */
#else
						/* ========= REV 6 ========== */
				/* notification to PC of first open of
				   a window */
#define FIRST_OPEN  _IOW(W, 53, int) 
				/* notification to PC of last close of
				   a window */
#define LAST_CLOSE  _IOW(W, 54, int) 
				 /* program execution list notification */
#define EXEC_LIST   _IOW(W, 55, Exec_list_buffsize )

				/* program doing a read with O_NDELAY set */
#define NDELAY_READ _IOW(W, 56, int) 
						/* ========= REV 6 ========== */
#endif
						/* ========= HP ============= */

						/* ======== WINDOWS_H ======= */
#endif
