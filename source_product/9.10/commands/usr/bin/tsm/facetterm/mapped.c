/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: mapped.c,v 70.1 92/03/09 15:45:11 ssa Exp $ */
/************************************************************************
* mapped.c
*	keyboard mapping
* window command mode
*	^W:h^U^Z		^U on any window sends ^Z to issuing window.
*	^W:m^Als -l		^A on any window sends ^A to current window.
*	^W:fMAPFILENAME		process mapping in MAPFILENAME
*	^W:u^U			unmap ^U
*
*	map-5=MAPFILENAME	in .facet ( not implemented )
**************************************************************************/
#include <sys/types.h>
#include <malloc.h>
					/**********************************
					Mapped_char is 1 if the character
					starts a string that is mapped.
					***********************************/
char	Mapped_char[ 256 ] = { 0 };
struct mapped 
{
	struct mapped	*next;
	char		*input_string;
	int		on_winno;
	char		*output_string;
	int		to_winno;
};

typedef struct mapped MAPPED;

MAPPED	*Mapped_start;

/**************************************************************************
* allocate_mapped
*	Allocate and return a keyboar mapping structure with storage for
*	strings of length "input_string_len" and "output_string_len".
*	Return a pointer which is 0 on failure.
**************************************************************************/
MAPPED *
allocate_mapped( input_string_len, output_string_len )
	int	input_string_len;
	int	output_string_len;
{
	MAPPED	*mapped;

	mapped = (MAPPED *) malloc( sizeof( MAPPED ) );
	if ( mapped == (MAPPED *) 0 )
	{
		return( (MAPPED *) 0 );
	}
	mapped->input_string = malloc( input_string_len + 1 );
	if ( mapped->input_string == (char *) 0 )
	{
		return( (MAPPED *) 0 );
	}
	mapped->output_string = malloc( output_string_len + 1 );
	if ( mapped->output_string == (char *) 0 )
	{
		return( (MAPPED *) 0 );
	}
	return( mapped );
}
/**************************************************************************
* install_mapped
*	Install a mapping specification
*	When the string "input_string" is typed on window "on_winno",
*	send the string "output_string" to window "to_winno".
*	The first character of "input_string" is set to 1 in 
*	Mapped_char.
**************************************************************************/
install_mapped( input_string, on_winno, output_string, to_winno )
	char	*input_string;
	int	on_winno;
	char	*output_string;
	int	to_winno;
{
	MAPPED	*mapped;
	MAPPED	*get_mapped();

	if ( ( mapped = get_mapped( input_string, on_winno ) ) != (MAPPED *) 0 )
	{
		free_mapped( mapped );
	}
	if ( ( mapped = 
	       allocate_mapped( strlen( input_string), strlen( output_string ) )
	     ) == ( MAPPED *) 0 )
	{
		return( -1 );
	}
	strcpy( mapped->input_string, input_string );
	mapped->on_winno = on_winno;
	strcpy( mapped->output_string, output_string );
	mapped->to_winno = to_winno;
					/* link at front of list */
	mapped->next = Mapped_start;
	Mapped_start = mapped;
					/* mark as beginning a mapped sequence*/
	Mapped_char[ input_string[ 0 ] ] = 1;
	return( 0 );
}
/**************************************************************************
* search_mapped
*	Check for a mapping specification
*	If string "input_string" is to be mapped when typed on window
*	on_winno, 
*	then store a pointer to the string at "p_output_string",
*	and store the destination window number at "p_to_winno".
*	Returns:
*		-1 = no match
*		0  = match
*		1  = partial match
**************************************************************************/
search_mapped( input_string, on_winno, p_output_string, p_to_winno )
	char	*input_string;
	int	on_winno;
	char	**p_output_string;
	int	*p_to_winno;
{
	MAPPED	*m;
	MAPPED	*get_mapped();

	m = get_mapped( input_string, on_winno );
	if ( m != (MAPPED *) 0 )
	{
		if ( strcmp( input_string, m->input_string ) == 0 )
		{
			*p_output_string = m->output_string;
			*p_to_winno     = m->to_winno;
			return( 0 );		/* match */
		}
		else
			return( 1 );		/* partial */
	}
	else
		return( -1 );			/* NO MATCH */
}
/**************************************************************************
* get_mapped
*	See if the string "input_string" occurring on window "on_winno"
*	is present in the currently active  mapping information.
*	Return a pointer to the mapping structure found
*	or 0 on not found.
**************************************************************************/
MAPPED *
get_mapped( input_string, on_winno )
	char	*input_string;
	int	on_winno;
{
	MAPPED	*m;
	int	len;

	len = strlen( input_string );
	for ( m = Mapped_start;
	      m != (MAPPED *) 0;
	      m = m->next )
	{
	    if (  ( m->on_winno == -1 ) || ( on_winno == m->on_winno ) )
	    {
		if ( strncmp( input_string, m->input_string, len ) == 0 )
		{
			return( m );		/* match or partial */
		}
	    }
	}
	return( m );			/* NO MATCH */
}
/**************************************************************************
* free_mapped
*	Unlink and release the mapping structure "mapped".
**************************************************************************/
free_mapped( mapped )
	MAPPED	*mapped;
{
	MAPPED	*last;
	MAPPED	*m;

	last = (MAPPED *) 0;
	for ( m = Mapped_start;
	      m != (MAPPED *) 0;
	      m = m->next )
	{
		if ( m == mapped )
		{
			if ( last == (MAPPED *) 0 )
				Mapped_start = m->next;
			else
				last->next = m->next;
			free( m );
			return;
		}
		last = m;
	}
}
/**************************************************************************
* unmap_mapped
*	Unmap any current mapping of the string "input_string" occurring
*	on window "to_winno".
**************************************************************************/
/*ARGSUSED*/
unmap_mapped( input_string, to_winno )
	char	*input_string;
	int	to_winno;
{
	MAPPED	*mapped;

	mapped = get_mapped( input_string, -1 );
	if ( mapped != (MAPPED *) 0 )
	{
		free_mapped( mapped );
	}
}
#ifdef lint
static int lint_alignment_warning_ok_1;
#endif
