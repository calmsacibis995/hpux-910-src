/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: tabs.c,v 70.1 92/03/09 15:47:09 ssa Exp $ */
/**************************************************************************
* tabs.c 
*	Provide support for tabs to terminals.
**************************************************************************/
#include <stdio.h>
#include "person.h"
#include "tabs_p.h"
#include "ftproc.h"
#include "ftwindow.h"
#include "features.h"
#include "decode.h"

char	*sT_clear_all_tabs = { NULL };

/**************************************************************************
* dec_clear_all_tabs
*	DECODE module for 'clear_all_tabs'.
**************************************************************************/
dec_clear_all_tabs()
{
	fct_clear_all_tabs();
}
/**************************************************************************
* inst_clear_all_tabs
*	INSTALL module for 'clear_all_tabs'.
**************************************************************************/
inst_clear_all_tabs( str )
	char	*str;
{
	dec_install( "clear_all_tabs", (UNCHAR *) str,
					dec_clear_all_tabs, 0, 0,
					(char *) 0 );
}
/**************************************************************************
* fct_clear_all_tabs
*	ACTION module for 'clear_all_tabs'.
**************************************************************************/
fct_clear_all_tabs()
{
	if ( Outwin->onscreen )
		term_clear_all_tabs();
}
/**************************************************************************
* term_clear_all_tabs
*	TERMINAL OUTPUT module for 'clear_all_tabs'.
**************************************************************************/
term_clear_all_tabs()
{
	if ( mT_clear_all_tabs != NULL )
	{
		term_tputs( mT_clear_all_tabs );
	}
}

char	*sT_set_tab = { NULL };

/**************************************************************************
* dec_set_tab
*	DECODE module for 'set_tab'.
**************************************************************************/
dec_set_tab()
{
	fct_set_tab();
}
/**************************************************************************
* inst_set_tab
*	INSTALL module for 'set_tab'.
**************************************************************************/
inst_set_tab( str )
	char	*str;
{
	dec_install( "set_tab", (UNCHAR *) str,
					dec_set_tab, 0, 0,
					(char *) 0 );
}
/**************************************************************************
* fct_set_tab
*	ACTION module for 'set_tab'.
**************************************************************************/
fct_set_tab()
{
	if ( Outwin->onscreen )
		term_set_tab();
}
/**************************************************************************
* term_set_tab
*	TERMINAL OUTPUT module for 'set_tab'.
**************************************************************************/
term_set_tab()
{
	if ( mT_set_tab != NULL )
	{
		term_tputs( mT_set_tab );
	}
}

char	*sT_clear_tab = { NULL };

/**************************************************************************
* dec_clear_tab
*	DECODE module for 'clear_tab'.
**************************************************************************/
dec_clear_tab()
{
	fct_clear_tab();
}
/**************************************************************************
* inst_clear_tab
*	INSTALL module for 'clear_tab'.
**************************************************************************/
inst_clear_tab( str )
	char	*str;
{
	dec_install( "clear_tab", (UNCHAR *) str,
					dec_clear_tab, 0, 0,
					(char *) 0 );
}
/**************************************************************************
* fct_clear_tab
*	ACTION module for 'clear_tab'.
**************************************************************************/
fct_clear_tab()
{
	if ( Outwin->onscreen )
		term_clear_tab();
}
/**************************************************************************
* term_clear_tab
*	TERMINAL OUTPUT module for 'clear_tab'.
**************************************************************************/
term_clear_tab()
{
	if ( mT_clear_tab != NULL )
	{
		term_tputs( mT_clear_tab );
	}
}

char	*sT_back_tab = { NULL };


/****************************************************************************/
char *dec_encode();
/**************************************************************************
* extra_tabs
*	TERMINAL DESCRIPTION PARSER module for 'tabs'.
**************************************************************************/
/*ARGSUSED*/
extra_tabs( buff, string, attr_on_string, attr_off_string ) 
	char	*buff;
	char	*string;
	char	*attr_on_string;		/* not used */
	char	*attr_off_string;		/* not used */
{
	char	*encoded;

	if ( strcmp( buff, "clear_all_tabs" ) == 0 )
	{
		xT_clear_all_tabs = dec_encode( string );
		inst_clear_all_tabs( xT_clear_all_tabs );
	}
	else if ( strcmp( buff, "set_tab" ) == 0 )
	{
		xT_set_tab = dec_encode( string );
		inst_set_tab( xT_set_tab );
	}
	else if ( strcmp( buff, "clear_tab" ) == 0 )
	{
		xT_clear_tab = dec_encode( string );
		inst_clear_tab( xT_clear_tab );
	}
	else if ( strcmp( buff, "back_tab" ) == 0 )
	{
		xT_back_tab = dec_encode( string );
		inst_back_tab( xT_back_tab );
	}
	else if ( strcmp( buff, "tab" ) == 0 )
	{
		encoded = dec_encode( string );
		inst_tab( encoded, 1 );
	}
	else
	{
		return( 0 );		/* no match */
	}
	return( 1 );
}
