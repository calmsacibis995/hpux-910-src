/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: foreign.c,v 70.1 92/03/09 16:15:57 ssa Exp $ */
/* #define DEBUG 1 */
#include "stdio.h"

#include <malloc.h>

#include "foreign.h"

#define FACETTEXT_MAX	256
#define TEXT_NAME_MAX	80

load_foreign( filename )
	char		*filename;
{
	FILE		*open_text();
	FILE		*facetfile;
	char		buffer[ FACETTEXT_MAX + 1 ];
	TEXTNAMES	*t;
	char		*ptr;
	char		c;
	char		*s;
	char		*d;
	int		len;
	char		name[ TEXT_NAME_MAX + 1 ];

	facetfile = open_text( filename );
	if ( facetfile == NULL )
		return;
	while ( read_dotfacet( buffer, FACETTEXT_MAX, facetfile ) >= 0 )
	{
		if ( buffer[ 0 ] != '!' )
			continue;
		strncpy( name, &buffer[ 1 ], TEXT_NAME_MAX );
		name[ TEXT_NAME_MAX ] = '\0';
		if ( read_dotfacet( buffer, FACETTEXT_MAX, facetfile ) < 0 )
			break;
		t = Textnames;
		while ( t->name[ 0 ] != '\0' )
		{
			if ( strcmp( t->name, name ) == 0 )
			{
				len = strlen( buffer );
				ptr = (char *) malloc( len + 1 );
				if ( ptr != NULL )
				{
					s = buffer;
					d = ptr;
					while( (c = *s++) != '\0' )
					{
						if ( c == '\\' )
						{
							if ( *s == 'r' )
							{
								*d++ = '\r';
								s++;
								continue;
							}
							else if ( *s == 'n' )
							{
								*d++ = '\n';
								s++;
								continue;
							}
							else if ( *s == 's' )
							{
								*d++ = ' ';
								s++;
								continue;
							}
							else if ( *s == '\\' )
							{
								*d++ = '\\';
								s++;
								continue;
							}
							else if ( *s == '0' )
							{
								*d++ = 0x80;
								s++;
								continue;
							}
						}
						*d++ = c;
					}
					*d = '\0';
					*(t->variable) = ptr;
				}
				break;
			}
			t++;
		}
	}
	fclose( facetfile );
}
