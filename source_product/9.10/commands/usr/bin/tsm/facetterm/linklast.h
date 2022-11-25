/*****************************************************************************
** Copyright (c)        1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: linklast.h,v 66.2 90/09/20 12:13:43 kb Exp $ */
struct t_struct_struct
{
	struct	t_struct_struct	*next;
};
typedef struct t_struct_struct T_STRUCT;
