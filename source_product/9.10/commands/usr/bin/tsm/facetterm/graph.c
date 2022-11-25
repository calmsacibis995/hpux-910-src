/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: graph.c,v 70.1 92/03/09 15:44:37 ssa Exp $ */
/**************************************************************************
* graph.c
*	Modules to support the "graph_mode_..." constructs to pass and
*	clear graphics for terminals with identifiable graphics sequences.
*	The program passes any sequences recognized by "graph_mode_start"
*	and sets a bit that there are graphics present.
*	The program passes any sequences recognized by "graph_mode_clear"
*	and clears the bit that there are graphics present.
*	When FacetTerm enters window command mode it outputs either 
*	"out_graph_mode_clear" or the last "graph_mode_clear" and
*	clears the bit that there are graphics present.
*	In this way, graphics will work on the current screen, and are
*	cleared before switching to another window but are not remembered
*	or refreshed.
**************************************************************************/
#include <stdio.h>
#include "ftproc.h"
#include "ftwindow.h"
#include "features.h"
#include "decode.h"

#define MAX_GRAPH_MODE_START	10
int	T_graph_mode_startno = 0;
char	*T_graph_mode_start[ MAX_GRAPH_MODE_START ]={NULL};

#define MAX_GRAPH_MODE_CLEAR	10
int	T_graph_mode_clearno = 0;
char	*T_graph_mode_clear[ MAX_GRAPH_MODE_START ]={NULL};

char	*T_out_graph_mode_clear = NULL;

int	M_graph_mode_on = 0;

/**************************************************************************
* init_windows_graph_mode
*	Initialize the graph mode of Outwin to no.
**************************************************************************/
init_windows_graph_mode()
{
	Outwin->graph_mode_on = 0;
}
/**************************************************************************
* modes_init_graph_mode
*	Initialize the graph mode of "win" to no.
**************************************************************************/
modes_init_graph_mode( win )
	FT_WIN  *win;
{
	win->graph_mode_on = 0;
}
/**************************************************************************
* dec_graph_mode_start
*	DECODE module for 'graph_mode_start'.
**************************************************************************/
/*ARGSUSED*/
dec_graph_mode_start( func_parm, parm_ptr, parms_valid, parm, string_parm )
	int	func_parm;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
	char	*string_parm[];
{
	fct_graph_mode_start( func_parm, string_parm[ 0 ] );
}
/**************************************************************************
* inst_graph_mode_start
*	INSTALL module for 'graph_mode_start'.
**************************************************************************/
inst_graph_mode_start( str, typeno )
	char	*str;
	int	typeno;
{
	dec_install( "graph_mode_start", (UNCHAR *) str, dec_graph_mode_start, 
		typeno, CURSOR_OPTION,
		(char *) 0 );
}
/**************************************************************************
* fct_graph_mode_start
*	ACTION module for 'graph_mode_start'.
**************************************************************************/
fct_graph_mode_start( typeno, string )
	int	typeno;
	char	*string;
{
	Outwin->graph_mode_on = 1;
	if ( Outwin->onscreen )
	{
		if ( Outwin->fullscreen )
			term_graph_mode_start( typeno, string );
		else
		{
			if ( outwin_is_curwin() )
			{
				modes_off_redo_screen();
				expand_to_full();
				modes_on_redo_screen();
				term_graph_mode_start( typeno, string );
			}
			else
			{
				force_split_outwin_offscreen();
				w_set_full( Outwin );
			}
		}
	}
	else
		w_set_full( Outwin );
}
extern	char	*my_tparm();
/**************************************************************************
* term_graph_mode_start
*	TERMINAL OUTPUT module for 'graph_mode_start'.
**************************************************************************/
term_graph_mode_start( typeno, string )
	int	typeno;
	char	*string;
{
	char	*graph_mode_startstr;
	char	*p;
	int	parm[ 2 ];			/* 1 used - must be >= 2 - %i */
	char	*string_parm[ 1 ];


	if ( typeno < T_graph_mode_startno )
	{
		graph_mode_startstr = T_graph_mode_start[ typeno ];
		if ( graph_mode_startstr != NULL )
		{
			string_parm[ 0 ] = string;
			p = my_tparm( graph_mode_startstr,
				parm, string_parm, -1 );
			term_tputs( p );
			M_graph_mode_on = 1;
		}
	}
}
/**************************************************************************
* dec_graph_mode_clear
*	DECODE module for 'graph_mode_clear'.
**************************************************************************/
/*ARGSUSED*/
dec_graph_mode_clear( func_parm, parm_ptr, parms_valid, parm, string_parm )
	int	func_parm;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
	char	*string_parm[];
{
	fct_graph_mode_clear( func_parm, string_parm[ 0 ] );
}
/**************************************************************************
* inst_graph_mode_clear
*	INSTALL module for 'graph_mode_clear'.
**************************************************************************/
inst_graph_mode_clear( str, typeno )
	char	*str;
	int	typeno;
{
	dec_install( "graph_mode_clear", (UNCHAR *) str, dec_graph_mode_clear, 
		typeno, CURSOR_OPTION,
		(char *) 0 );
}
/**************************************************************************
* fct_graph_mode_clear
*	ACTION module for 'graph_mode_clear'.
**************************************************************************/
fct_graph_mode_clear( typeno, string )
	int	typeno;
	char	*string;
{
	Outwin->graph_mode_on = 1;
	if ( Outwin->onscreen )
		term_graph_mode_clear( typeno, string );
}
/**************************************************************************
* term_graph_mode_clear
*	TERMINAL OUTPUT module for 'graph_mode_clear'.
**************************************************************************/
term_graph_mode_clear( typeno, string )
	int	typeno;
	char	*string;
{
	char	*graph_mode_clearstr;
	char	*p;
	int	parm[ 2 ];			/* 1 used - must be >= 2 - %i */
	char	*string_parm[ 1 ];


	if ( typeno < T_graph_mode_clearno )
	{
		graph_mode_clearstr = T_graph_mode_clear[ typeno ];
		if ( graph_mode_clearstr != NULL )
		{
			string_parm[ 0 ] = string;
			p = my_tparm( graph_mode_clearstr,
				parm, string_parm, -1 );
			term_tputs( p );
			M_graph_mode_on = 0;
		}
		else
			term_beep();
	}
	else
		term_beep();
}
/**************************************************************************
* term_out_graph_mode_clear
*	This is an internally generated request.
*	Output a sequence to remove the graphics from the terminal screen.
**************************************************************************/
term_out_graph_mode_clear()
{
	if ( T_out_graph_mode_clear != NULL )
	{
		term_tputs( T_out_graph_mode_clear );
		M_graph_mode_on = 0;
	}
	else if ( T_graph_mode_clearno > 0 )
	{
		term_graph_mode_clear( T_graph_mode_clearno - 1, "" );
	}
	else
		term_beep();
}
/**************************************************************************
* t_sync_graph_mode
*	Syncronize the terminal to the graphics mode "graph_mode_on".
*	Since we are not refreshing graphics,
*	the only action is to clear graphics if they are on
*	and "graph_mode_on" is 0.
**************************************************************************/
t_sync_graph_mode( graph_mode_on )
	int	graph_mode_on;
{
	if ( graph_mode_on )
	{
	}
	else
	{
		if ( M_graph_mode_on )
			term_out_graph_mode_clear();
	}
}
char *dec_encode();
/**************************************************************************
* extra_graph_mode
*	TERMINAL DESCRIPTION PARSER module for 'graph_mode_...'.
**************************************************************************/
/*ARGSUSED*/
extra_graph_mode( buff, string, attr_on_string, attr_off_string ) 
	char	*buff;
	char	*string;
	char	*attr_on_string;		/* not used */
	char	*attr_off_string;		/* not used */
{
	if ( strcmp( buff, "graph_mode_start" ) == 0 )
	{
		if ( T_graph_mode_startno < MAX_GRAPH_MODE_START )
		{
			T_graph_mode_start[ T_graph_mode_startno ] =
				dec_encode( string );
			inst_graph_mode_start(
				T_graph_mode_start[ T_graph_mode_startno ],
				T_graph_mode_startno );
			T_graph_mode_startno++;
		}
		else
		{
			printf( "Too many graph_mode_start\n" );
		}
	}
	else if ( strcmp( buff, "graph_mode_clear" ) == 0 )
	{
		if ( T_graph_mode_clearno < MAX_GRAPH_MODE_CLEAR )
		{
			T_graph_mode_clear[ T_graph_mode_clearno ] =
				dec_encode( string );
			inst_graph_mode_clear(
				T_graph_mode_clear[ T_graph_mode_clearno ],
				T_graph_mode_clearno );
			T_graph_mode_clearno++;
		}
		else
		{
			printf( "Too many graph_mode_clear\n" );
		}
	}
	else if ( strcmp( buff, "out_graph_mode_clear" ) == 0 )
	{
			T_out_graph_mode_clear = dec_encode( string );
	}
	else
	{
		return( 0 );		/* no match */
	}
	return( 1 );
}
