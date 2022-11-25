/*****************************************************************************
** Copyright (c)        1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: tixer.h,v 70.1 92/03/09 16:14:08 ssa Exp $ */
/*
 * File:		sdp.h
 * Creator:		G.Clark.Brown
 *
 * Structures, typedefs and global data for the Terminfo Exerciser Program.
 *
 * Module name: 	%M%
 * SID:			%I%
 * Extract date:	%H% %T%
 * Delta date:		%G% %U%
 * Stored at:		%P%
 * Module type:		%Y%
 * 
 */
struct	cap_table_type
{
	char	*variable_name;
	char	*capname;
	char	*icode;
	char	*description;
	int	requires_cup;
	int	type;
	int	(*routine)();
	int	(*dup_routine)();
};

struct	option_tab
{
	int			test;
	char			type[80];
	int			captab;
	int			runaway;	/* run without pausing? */
	char			command[512];	/* command to exec at pause */
};

