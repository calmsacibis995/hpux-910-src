/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: commonread.c,v 70.2 92/05/28 15:10:58 ssa Exp $ */
#include <stdio.h>

#include "facetpath.h"

/**************************************************************************
* open_dotfacet
*	Locate and open the file filename in the following directories:
*		current working directory (of the facetterm program)
*		$HOME
*		/usr/facetterm
*		/usr/facet
*	Return the stream pointer or NULL.
**************************************************************************/
FILE *
open_dotfacet( filename )
	char	*filename;
{
	FILE	*facetfile;
	char	*home;
	char	pathname[ 256 ];
	char	*getenv();

	facetfile = fopen( filename, "r" );
	if ( facetfile == NULL )
	{
		home = getenv( "HOME" );
		if ( home != NULL )
		{
			strcpy( pathname, home );
			strcat( pathname, "/" );
			strcat( pathname, filename );
			facetfile = fopen( pathname, "r" );
		}
	}
	if ( facetfile == NULL )
	{
		sprintf( pathname,"%s/%s", Facettermpath, filename );
		facetfile = fopen( pathname, "r" );
	}
	if ( facetfile == NULL )
	{
		sprintf( pathname,"%s/%s", Facetpath, filename );
		facetfile = fopen( pathname, "r" );
	}
	return( facetfile );
}
/**************************************************************************
* read_dotfacet
*	Read a line from a .facet file "facetfile" and return a maximum
*	of "max" characters in "buffer".
*	Ignore lines that:
*		have 0 length
*		consist of only a newline
*		begin with #
*	Trim off the trailing newline, if any.
*	Return -1 on end of file, 0 otherwise.
**************************************************************************/
read_dotfacet( buffer, max, facetfile )
	char	*buffer;
	int	max;
	FILE	*facetfile;
{
	int	len;

	while ( fgets( buffer, max, facetfile ) != NULL )
	{
		len = strlen( buffer );
		if (  ( len < 1 ) 
		   || ( ( len == 1 ) && ( buffer[ 0 ] == '\n' ) )
		   || ( buffer[ 0 ] == '#' ) )
		{
			continue;
		}
		if ( buffer[ len-1 ] == '\n' )
			buffer[ len-1 ] = '\0';
		return( 0 );
	}
	return( -1 );
}
/******************************************************************
* open_text
*	look in  .  $FACETTEXT/$LANG  ($FACETTEXT/C on HP)  $FACETTEXT and $HOME
*	look in  ./.tsmtext $HOME/.tsmtext /usr/facetterm/text /usr/facet/text
*	with possible subdirectories $LANG ( and "C" on HP ).
*	for the file "filename".
*	Return FILE ptr to stream or NULL.
******************************************************************/
FILE *
open_text( filename )
	char	*filename;
{
	FILE	*facetfile;
	char	pathname[ 256 ];
	char	*getenv();
	char	*home;
	char	*lang;
	char	*langC;
	char	*facettext;

	home = getenv( "HOME" );
	lang = getenv( "LANG" );
	langC = "C";
	if (  ( lang  == (char *) 0      ) 
	   || ( *lang != '\0'            )
	   || ( strcmp( lang, "C" ) == 0 )
	   ) 
	{
		lang = (char *) 0;
	}
	facetfile = NULL;
	facettext = getenv( "FACETTEXT" );
	if ( facettext == NULL )
		facettext = getenv( "TSMTEXT" );
	if ( facettext != NULL )
	{
	    if ( ( facetfile == NULL ) && ( lang != (char *) 0 ) )
	    {
				 /* $FACETTEXT/$LANG/filename */
		sprintf( pathname, "%s/%s/%s", 
				   facettext, lang, filename );
		facetfile = fopen( pathname, "r" );
	    }
	    if ( ( facetfile == NULL ) && ( langC != (char *) 0 ) )
	    {
				 /* $FACETTEXT/C/filename */
		sprintf( pathname, "%s/%s/%s", 
				   facettext, langC, filename );
		facetfile = fopen( pathname, "r" );
	    }
	    if ( facetfile == NULL )
	    {
				 /* $FACETTEXT/filename */
		sprintf( pathname, "%s/%s", 
				   facettext, filename );
		facetfile = fopen( pathname, "r" );
	    }
	}
	if ( ( facetfile == NULL ) && ( lang != (char *) 0 ) )
	{
				 /* ./.facettext/$LANG/filename */
		sprintf( pathname, "./.%stext/%s/%s", 
				   Facetname, lang, filename );
		facetfile = fopen( pathname, "r" );
	}
	if ( ( facetfile == NULL ) && ( langC != (char *) 0 ) )
	{
				 /* ./.facettext/C/filename */
		sprintf( pathname, "./.%stext/%s/%s", 
				   Facetname, langC, filename );
		facetfile = fopen( pathname, "r" );
	}
	if ( facetfile == NULL )
	{
				 /* ./.facettext/filename */
		sprintf( pathname, "./.%stext/%s", 
				   Facetname, filename );
		facetfile = fopen( pathname, "r" );
	}
	if ( ( facetfile == NULL ) && ( lang != (char *) 0 ) )
	{
				 /* $HOME/.facettext/$LANG/filename */
		sprintf( pathname, "%s/.%stext/%s/%s", 
				   home, Facetname, lang, filename );
		facetfile = fopen( pathname, "r" );
	}
	if ( ( facetfile == NULL ) && ( langC != (char *) 0 ) )
	{
				 /* $HOME/.facettext/C/filename */
		sprintf( pathname, "%s/.%stext/%s/%s", 
				   home, Facetname, langC, filename );
		facetfile = fopen( pathname, "r" );
	}
	if ( facetfile == NULL )
	{
				 /* $HOME/.facettext/filename */
		sprintf( pathname, "%s/.%stext/%s", 
				   home, Facetname, filename );
		facetfile = fopen( pathname, "r" );
	}
	if ( ( facetfile == NULL ) && ( lang != (char *) 0 ) )
	{
				 /* /usr/facetterm/text/$LANG/filename */
		sprintf( pathname, "%s/text/%s/%s", 
				   Facettermpath, lang, filename );
		facetfile = fopen( pathname, "r" );
	}
	if ( ( facetfile == NULL ) && ( langC != (char *) 0 ) )
	{
				 /* /usr/facetterm/text/C/filename */
		sprintf( pathname, "%s/text/%s/%s", 
				   Facettermpath, langC, filename );
		facetfile = fopen( pathname, "r" );
	}
	if ( facetfile == NULL )
	{
				 /* /usr/facetterm/text/filename */
		sprintf( pathname, "%s/text/%s", 
				   Facettermpath, filename );
		facetfile = fopen( pathname, "r" );
	}
	if ( ( facetfile == NULL ) && ( lang != (char *) 0 ) )
	{
				 /* /usr/facet/text/$LANG/filename */
		sprintf( pathname, "%s/text/%s/%s", 
				   Facetpath, lang, filename );
		facetfile = fopen( pathname, "r" );
	}
	if ( ( facetfile == NULL ) && ( langC != (char *) 0 ) )
	{
				 /* /usr/facet/text/C/filename */
		sprintf( pathname, "%s/text/%s/%s", 
				   Facetpath, langC, filename );
		facetfile = fopen( pathname, "r" );
	}
	if ( facetfile == NULL )
	{
				 /* /usr/facet/text/filename */
		sprintf( pathname, "%s/text/%s", 
				   Facetpath, filename );
		facetfile = fopen( pathname, "r" );
	}
	return( facetfile );
}
