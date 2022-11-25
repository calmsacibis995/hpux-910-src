/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: set.c,v 70.1 92/03/09 15:46:53 ssa Exp $ */
/**************************************************************************
* set.c
*	Support terminal description file capability
*		set-name=sequence
*	such that a function key file can have a line
*		name contents
*	and the value of 'contents' will be applied to 'sequence' and
*	fed into the decoder for the window.
**************************************************************************/
#include <stdio.h>
#include "ftproc.h"
#include "ftwindow.h"
#include "features.h"
#include "decode.h"
#include "wins.h"

extern int errno;

struct set
{
	struct set	*next;
	char		*name;
	char		*string;
};
typedef struct set SET;

/****************************************************************************/

SET	*Set_root = NULL;
/**************************************************************************
* extra_set
*	TERMINAL DESCRIPTION PARSER module for 'set'.
**************************************************************************/
/*ARGSUSED*/
extra_set( buff, string, attr_on_string, attr_off_string ) 
	char	*buff;
	char	*string;
	char	*attr_on_string;
	char	*attr_off_string;		/* not used */
{
	char		*dec_encode();
	SET		*newset;
	SET		*pset;
	int		name_length;
	char		*name;
	long		*malloc_run();

	if ( strcmp( buff, "set" ) == 0 )
	{
		if ( ( attr_on_string == NULL ) || ( *attr_on_string == '\0' ) )
		{
			printf( "set has missing name\n" );
			return( 1 );
		}
		name_length = strlen( attr_on_string );
		if ( ( string == NULL ) || ( *string == '\0' ) )
		{
			printf( "set has missing contents\n" );
			return( 1 );
		}
		newset = ( SET * ) malloc_run( sizeof( SET ), "set_structure" );
		if ( newset == NULL )
		{
			printf( "ERROR: Set malloc failed - %d\n",
				errno );
			return( 1 );
		}
		name = (char * ) malloc_run( name_length + 1, "set_name" );
		if ( name == NULL )
		{
			printf( "ERROR: Set name malloc failed - %d\n",
				errno );
			return( 1 );
		}
		strcpy( name, attr_on_string );
		newset->next = NULL;
		newset->name = name;
		newset->string = dec_encode( string );
		if ( Set_root == NULL )
			Set_root = newset;
		else
		{
			pset = Set_root;
			while ( pset->next != NULL )
				pset = pset->next;
			pset->next = newset;
		}
	}
	else
	{
		return( 0 );		/* no match */
	}
	return( 1 );
}
#include "max_buff.h"
/**************************************************************************
* store_set
*	See if "name" matches a "set" capability from the terminal
*	description file.
*	If so apply "contents" to the sequence specified in the terminal
*	description file to windows numbered "first" to "last" and
*	return 0
*	Otherwise return -1;
**************************************************************************/
store_set( first, last, name, contents )
	int	first;
	int	last;
	char	*name;
	char	*contents;
{
	SET	*pset;
	char	*my_tparm();
	char	*p;
	int	parm[ 2 ];			/* 1 used - must be >= 2 - %i */
	char	*string_parm[ 1 ];
	int	winno;
	char	buffer[ MAX_BUFF + 300 ];
	char	outbuffer[ MAX_BUFF + 300 ];

	for ( pset = Set_root; pset != NULL; pset = pset->next )
	{
		if ( strcmp( name, pset->name ) == 0 )
		{
			for ( winno = first; winno <= last; winno++ )
			{
				sprintf( buffer, contents, winno + 1 );
				string_parm[ 0 ] = buffer;
				p = my_tparm( pset->string, parm, string_parm,
									winno );
				strncpy( outbuffer, p, MAX_BUFF );
				outbuffer[ MAX_BUFF ] = '\0';
				fct_decode_string( winno, outbuffer );
			}
			return( 0 );
		}
	}
	return( -1 );
}
#ifdef lint
static int lint_alignment_warning_ok_1;
#endif
