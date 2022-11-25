/*****************************************************************************
** Copyright (c) 1990        Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: commandmap.c,v 70.1 92/03/09 15:41:18 ssa Exp $ */
/**************************************************************************
* command_map
*	Determine if the character "cc" is present in the mapping string
*	"mapping".
*	This is used in conjuction with the foreign language text files
*	to map keyboard responses into the characters expected by the
*	program.
*	The mappings consists of multiples of:
*		1. a character
*		2. a string which is to replace that character - terminated
*		   by 0x80.
*	The entire mapping is terminated with a NULL.
*	Return a pointer to the string to which "cc" is to be mapped -
*	or return NULL if it is not to be mapped.
**************************************************************************/
char	*
command_map( cc, mapping )
	int	cc;
	char	*mapping;
{
	char	*p;
	int	map_char;

	p = mapping;
	while( *p != '\0' )
	{
		/**********************************************************
		* If this is the correct mapping, return a pointer to
		* the string after it.
		**********************************************************/
		map_char =  *p++ & 0x00FF;
		if ( cc == map_char )
			return( p );
		/**********************************************************
		* If not correct, skip over the string following it.
		**********************************************************/
		while( *p != '\0' )
		{
			if ( ( *p++ & 0x00FF ) == 0x0080 )
				break;
		}
	}
	/******************************************************************
	* Not a mapped char.
	******************************************************************/
	p = (char *) 0;
	return( p );
}
/**************************************************************************
* map_prompt
*	Determine if the character "prompt_char" is present in the
*	mapping string "mapping".
*	This is used in conjuction with the foreign language text files
*	to map a default response into the character expected by the
*	person.
*	The mappings consists of multiples of:
*		1. a character
*		2. a string which is to replace that character - terminated
*		   by 0x80.
*	The entire mapping is terminated with a NULL.
*	Only the first character of the string is used for a prompt.
*	Return the prompt character - remapped or not.
**************************************************************************/
char
map_prompt( prompt_char, mapping )
	char	prompt_char;
	char	*mapping;
{
	char	*p;
	char	result;

	result = prompt_char;
	p = command_map( (int) prompt_char, mapping );
	if ( p != (char *) 0 )
		result = *p;
	return( result );
}
