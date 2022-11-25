/*****************************************************************************
** Copyright (c)        1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: tixerext.h,v 70.1 92/03/09 16:14:12 ssa Exp $ */
/*
 * File:		tixerext.h
 * Creator:		G.Clark.Brown
 * Creation Date:	10/7/88
 *
 * External data definitions for Terminfo Exerciser Program.
 *
 * Module name: 	%M%
 * SID:			%I%
 * Extract date:	%H% %T%
 * Delta date:		%G% %U%
 * Stored at:		%P%
 * Module type:		%Y%
 * 
 */


extern char			tixerdir[257];	/* path for script files */
extern struct stat		Statbuf;
extern char			*u_strtok();
extern char			*getenv();
extern FILE			*u_fopen();
extern FILE			*u_popen();
extern FILE			*get_device();
extern FILE			*check_device();

extern struct cap_table_type	cap_table[];

extern int			tc_putchar();
