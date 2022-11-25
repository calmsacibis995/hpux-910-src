/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: substitute.c,v 70.1 92/03/09 16:44:29 ssa Exp $ */
/**************************************************************************
* Modules supporting substitutes such as reverse video
**************************************************************************/
#include <stdio.h>
#include "ftchar.h"
#include "ftproc.h"
#include "ftwindow.h"
#include "options.h"

#include "decode.h"

typedef struct t_substitute_struct T_SUBSTITUTE;
struct t_substitute_struct
{
	T_SUBSTITUTE		*next;
	char			*t_substitute_new;
};
T_SUBSTITUTE	*T_substitute_ptr = { (T_SUBSTITUTE *) 0 };

char	*T_substitute_new = (char *) 0;
#include "linklast.h"
/**************************************************************************
* dec_substitute
*	DECODE module for 'substitute'.
**************************************************************************/
/*ARGSUSED*/
dec_substitute( unused, t_substitute_ptr, parms_valid, parm,
					string_parm, string_parms_valid )
	int		unused;
	T_SUBSTITUTE	*t_substitute_ptr;
	int		parms_valid;
	int		parm[];
	UNCHAR		*string_parm[];
	int		string_parms_valid;
{
	fct_substitute( t_substitute_ptr,
			parms_valid, parm, string_parm, string_parms_valid );
}
/**************************************************************************
* inst_substitute
**************************************************************************/
inst_substitute( str, parmno, t_substitute_ptr )
	char		*str;
	int		parmno;
	T_SUBSTITUTE	*t_substitute_ptr;
{
	dec_install( "substitute", (UNCHAR *) str,
		dec_substitute, parmno, 0,
		(char *) t_substitute_ptr );
}
#include "max_buff.h"
/**************************************************************************
* fct_substitute
*	ACTION module for 'substitute'.
**************************************************************************/
#define MAX_BUFF_SUB	(MAX_BUFF + 300)
fct_substitute( t_substitute_ptr,
			parms_valid, parm, string_parm, string_parms_valid )
	T_SUBSTITUTE	*t_substitute_ptr;
	int		parms_valid;
	int		parm[];
	UNCHAR		*string_parm[];
	int		string_parms_valid;
{
	char	*p;
	char	*s;
	char	buff[ MAX_BUFF_SUB + 1 ];
	char	*my_tparm();

	p = my_tparm( t_substitute_ptr->t_substitute_new, parm, string_parm,
							Outwin->number );
	strncpy( buff, p, MAX_BUFF_SUB );
	buff[ MAX_BUFF_SUB ] = '\0';
	start_substitute();
	for ( s = buff; *s != '\0'; s++ )
	{
		chk_char( *s );
	}
	end_substitute();
}
/**************************************************************************
* extra_substitute
*	Examine terminal description file line for info relevant to
*	substitutes and install if found.
**************************************************************************/
extra_substitute( name, string, substitute_on_string, substitute_off_string )
	char	*name;
	char	*string;
	char	*substitute_on_string;
	char	*substitute_off_string;
{
	int	match;
	char	*dec_encode();

	match = 1;
	if ( strcmp( name, "substitute_new" ) == 0 )
	{
		T_substitute_new = dec_encode( string );
	}
	else if ( strcmp( name, "substitute_for" ) == 0 )
	{
		T_SUBSTITUTE	*t_substitute_ptr;
		long		*mymalloc();
		char		*encoded;

		if ( T_substitute_new == (char *) 0 )
		{
			printf( "substitute_for preceeds substitute_new\n" );
			return( 1 );
		}
		t_substitute_ptr = 
			(T_SUBSTITUTE *) mymalloc( sizeof( T_SUBSTITUTE ), 
					  "substitute" );
		t_substitute_ptr->next = (T_SUBSTITUTE *) 0;
		link_last( (T_STRUCT *) t_substitute_ptr,
			   (T_STRUCT *) &T_substitute_ptr );
		t_substitute_ptr->t_substitute_new = T_substitute_new;
		encoded = dec_encode( string );
		inst_substitute( encoded, 0, t_substitute_ptr );
	}
	else
		return( 0 );
	return( 1 );
}
