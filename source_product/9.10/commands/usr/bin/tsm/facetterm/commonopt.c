/*****************************************************************************
** Copyright (c) 1990        Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: commonopt.c,v 66.5 90/09/20 12:05:45 kb Exp $ */
#include <stdio.h>
#include "facetpath.h"

char	*Facetpath = "/usr/tsm";
char	*Facettermpath = "/usr/tsm";
char	*Facetprefix= "tsm.";
char	*Facetname= "tsm";

/**************************************************************************
* set_options_facetpath
*	Set the location and naming variables for facetterm based on
*	environment variables.
*	Facetpath
*		/usr/facet	Files common to FACET/TERM and FACET/PC
*	Facettermpath
*		/usr/facetterm	Files that are FACET/TERM only.
*	Facetprefix
*		fct_	program prefix	E.G.	fct_key	fct_command
*	Facetname
*		facet	filename prefix	E.G.	.facet	.facettext
**************************************************************************/
set_options_facetpath()
{
	char    *p;
	char    *getenv();

	p = getenv( "FACETPATH" );
	if ( p != NULL )
		Facetpath = p;

	p = getenv( "TSMPATH" );
	if ( p != NULL )
	{
		Facetpath = p;
		Facettermpath = p;
	}

	p = getenv( "FACETTERMPATH" );
	if ( p != NULL )
		Facettermpath = p;

	p = getenv( "TSMTERMPATH" );
	if ( p != NULL )
		Facettermpath = p;

	p = getenv( "FACETPREFIX" );
	if ( p != NULL )
		Facetprefix = p;

	p = getenv( "TSMPREFIX" );
	if ( p != NULL )
		Facetprefix = p;

	p = getenv( "FACETNAME" );
	if ( p != NULL )
		Facetname = p;

	p = getenv( "TSMNAME" );
	if ( p != NULL )
		Facetname = p;
}
