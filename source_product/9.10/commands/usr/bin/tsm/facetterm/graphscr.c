/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: graphscr.c,v 70.1 92/03/09 16:40:25 ssa Exp $ */
/**************************************************************************
* graphscr.c
*	Modules to support the "graph_screen_..." constructs to pass and
*	clear graphics for terminals with identifiable graphics sequences.
*	The program passes any sequences recognized by "graph_screen_on"
*	and sets a bit that there are graphics present.
*	The program passes any sequences recognized by "graph_screen_off"
*	and clears the bit that there are graphics present.
*	When FacetTerm enters window command mode it outputs either 
*	"out_graph_screen_off" or the last "graph_screen_off" and
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
extern int	errno;

#define MAX_GRAPH_SCREEN_START	10
int	T_graph_screen_onno = 0;
char	*T_graph_screen_on[ MAX_GRAPH_SCREEN_START ]={NULL};

char	*T_out_graph_screen_on = NULL;

char	*T_graph_screen_off = { NULL };

char	*T_out_graph_screen_off = { NULL };

int	M_graph_screen_on = 0;

#define	GRAPH_SCREEN_SE_MAX 40
long	F_graph_screen_on_side_effect[ GRAPH_SCREEN_SE_MAX + 1 ] = { 0 };
long	F_graph_screen_off_side_effect[ GRAPH_SCREEN_SE_MAX + 1 ] = { 0 };

/**************************************************************************
* init_windows_graph_screen
*	Initialize the graph screen of Outwin to no.
**************************************************************************/
init_windows_graph_screen()
{
	Outwin->graph_screen_on = 0;
}
/**************************************************************************
* modes_init_graph_screen
*	Initialize the graph screen of "win" to no.
**************************************************************************/
modes_init_graph_screen( win )
	FT_WIN  *win;
{
	win->graph_screen_on = 0;
}
/**************************************************************************
* dec_graph_screen_on
*	DECODE module for 'graph_screen_on'.
**************************************************************************/
/*ARGSUSED*/
dec_graph_screen_on( func_parm, parm_ptr, parms_valid, parm, string_parm )
	int	func_parm;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
	char	*string_parm[];
{
	fct_graph_screen_on( func_parm, string_parm[ 0 ] );
}
/**************************************************************************
* inst_graph_screen_on
*	INSTALL module for 'graph_screen_on'.
**************************************************************************/
inst_graph_screen_on( str, typeno )
	char	*str;
	int	typeno;
{
	dec_install( "graph_screen_on", (UNCHAR *) str, dec_graph_screen_on, 
		typeno, CURSOR_OPTION,
		(char *) 0 );
}
/**************************************************************************
* fct_graph_screen_on
*	ACTION module for 'graph_screen_on'.
**************************************************************************/
fct_graph_screen_on( typeno, string )
	int	typeno;
	char	*string;
{
	if ( Outwin->graph_screen_on )
		return;
	Outwin->graph_screen_on = 1;
	set_row_changed_all( Outwin );
	set_col_changed_all( Outwin );
	win_se( F_graph_screen_on_side_effect );
	if ( Outwin->onscreen )
	{
		if ( Outwin->fullscreen )
			term_graph_screen_on( typeno, string );
		else
		{
			if ( outwin_is_curwin() )
			{
				modes_off_redo_screen();
				expand_to_full();
				modes_on_redo_screen();
				term_graph_screen_on( typeno, string );
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
* term_graph_screen_on
*	TERMINAL OUTPUT module for 'graph_screen_on'.
**************************************************************************/
term_graph_screen_on( typeno, string )
	int	typeno;
	char	*string;
{
	char	*graph_screen_onstr;
	char	*p;
	int	parm[ 2 ];			/* 1 used - must be >= 2 - %i */
	char	*string_parm[ 1 ];


	if ( typeno < T_graph_screen_onno )
	{
		graph_screen_onstr = T_graph_screen_on[ typeno ];
		if ( graph_screen_onstr != NULL )
		{
			string_parm[ 0 ] = string;
			p = my_tparm( graph_screen_onstr,
				parm, string_parm, -1 );
			term_tputs( p );
			M_graph_screen_on = 1;
			term_se( F_graph_screen_on_side_effect );
		}
	}
}
/**************************************************************************
* term_out_graph_screen_on
*	This is an internally generated request.
*	Output a sequence change the terminal to a graphics screen.
**************************************************************************/
term_out_graph_screen_on()
{
	if ( T_out_graph_screen_on != NULL )
	{
		term_tputs( T_out_graph_screen_on );
		M_graph_screen_on = 1;
		term_se( F_graph_screen_on_side_effect );
	}
	else
		term_beep();
}
/**************************************************************************
* dec_graph_screen_off
*	DECODE module for 'graph_screen_off'.
**************************************************************************/
/*ARGSUSED*/
dec_graph_screen_off( func_parm, parm_ptr, parms_valid, parm, string_parm )
	int	func_parm;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
	char	*string_parm[];
{
	fct_graph_screen_off();
}
/**************************************************************************
* inst_graph_screen_off
*	INSTALL module for 'graph_screen_off'.
**************************************************************************/
inst_graph_screen_off( str, unused )
	char	*str;
	int	unused;
{
	dec_install( "graph_screen_off", (UNCHAR *) str, dec_graph_screen_off, 
		0, CURSOR_OPTION,
		(char *) 0 );
}
/**************************************************************************
* fct_graph_screen_off
*	ACTION module for 'graph_screen_off'.
**************************************************************************/
fct_graph_screen_off()
{
	if ( Outwin->graph_screen_on == 0 )
		return;
	Outwin->graph_screen_on = 0;
	win_se( F_graph_screen_off_side_effect );
	if ( Outwin->onscreen )
	{
		term_graph_screen_off();
		term_change_scroll_region( Outwin->csr_buff_top_row,
			 		Outwin->csr_buff_bot_row);
	}
}
/**************************************************************************
* term_graph_screen_off
*	TERMINAL OUTPUT module for 'graph_screen_off'.
**************************************************************************/
term_graph_screen_off()
{
	if ( T_out_graph_screen_off != NULL )
	{
		term_tputs( T_out_graph_screen_off );
		M_graph_screen_on = 0;
		term_se( F_graph_screen_off_side_effect );
	}
	else
		term_beep();
}
/**************************************************************************
* term_out_graph_screen_off
*	This is an internally generated request.
*	Output a sequence to remove the graphics from the terminal screen.
**************************************************************************/
term_out_graph_screen_off()
{
	if ( T_out_graph_screen_off != NULL )
	{
		term_tputs( T_out_graph_screen_off );
		M_graph_screen_on = 0;
		term_se( F_graph_screen_off_side_effect );
	}
	else
		term_beep();
}

/**************************************************************************
**************************************************************************/
#include "wins.h"

typedef struct 
{
	int	off_matched;
	char	*storage;
	int	in_storage;
} GRAPHSCREEN;

GRAPHSCREEN	Graph_screen[ TOPWIN ] = { 0 };
int		Graph_screen_storage_max = 4096;
/**************************************************************************
* graph_screen_char
**************************************************************************/
graph_screen_char( c )
	int	c;
{
	long		*malloc_run();
	GRAPHSCREEN	*g;

	g = &Graph_screen[ Outwin->number ];
	if ( Outwin->onscreen )
	{
		term_putc( c );
	}
	if ( g->storage == NULL )
	{
		g->storage = (char *) malloc_run( Graph_screen_storage_max + 1,
						  "graph_screen" );
		if ( g->storage == NULL )
		{
			term_outgo();
			printf( "ERROR: Graph screen malloc failed - %d\r\n",
				errno );
			term_outgo();
		}
	}
	if ( g->storage != NULL )
	{
		if ( g->in_storage < Graph_screen_storage_max )
			g->storage[ g->in_storage++ ] = c;
	}
	if ( c == T_graph_screen_off[ g->off_matched ] )
	{
		g->off_matched++;
		if ( g->off_matched >= strlen( T_graph_screen_off ) )
		{
			fct_graph_screen_off();
			g->off_matched = 0;
			g->in_storage = 0;
		}
	}
	else
		g->off_matched = 0;
}
term_graph_screen_reload()
{
	GRAPHSCREEN	*g;

	g = &Graph_screen[ Outwin->number ];
	if ( g->storage != NULL )
	{
		if ( g->in_storage < Graph_screen_storage_max )
		{
			term_write( g->storage, g->in_storage );
		}
	}
}
/**************************************************************************
* graph_screen_off_window_mode
**************************************************************************/
graph_screen_off_window_mode()
{
	if ( M_graph_screen_on )
	{
		term_out_graph_screen_off();
		term_reload_scroll_region_normal();
	}
}
/**************************************************************************
* graph_screen_on_window_mode
**************************************************************************/
graph_screen_on_window_mode()
{
	if ( Outwin->graph_screen_on )
	{
		term_out_graph_screen_on();
		term_graph_screen_reload();
	}
}
char *dec_encode();
/**************************************************************************
* extra_graph_screen
*	TERMINAL DESCRIPTION PARSER module for 'graph_screen_...'.
**************************************************************************/
/*ARGSUSED*/
extra_graph_screen( buff, string, attr_on_string, attr_off_string ) 
	char	*buff;
	char	*string;
	char	*attr_on_string;		/* not used */
	char	*attr_off_string;		/* not used */
{
	if ( strcmp( buff, "graph_screen_on" ) == 0 )
	{
		if ( T_graph_screen_onno < MAX_GRAPH_SCREEN_START )
		{
			T_graph_screen_on[ T_graph_screen_onno ] =
				T_out_graph_screen_on =
				dec_encode( string );
			inst_graph_screen_on(
				T_graph_screen_on[ T_graph_screen_onno ],
				T_graph_screen_onno );
			T_graph_screen_onno++;
		}
		else
		{
			printf( "Too many graph_screen_on\n" );
		}
	}
	else if ( strcmp( buff, "out_graph_screen_on" ) == 0 )
	{
			T_out_graph_screen_on = dec_encode( string );
	}
	else if ( strcmp( buff, "graph_screen_off" ) == 0 )
	{
		T_graph_screen_off =
			T_out_graph_screen_off =
			dec_encode( string );
	}
	else if ( strcmp( buff, "out_graph_screen_off" ) == 0 )
	{
		T_out_graph_screen_off = dec_encode( string );
	}
	else if ( strcmp( buff, "graph_screen_storage_max" ) == 0 )
	{
		Graph_screen_storage_max = atoi( string );
		if ( Graph_screen_storage_max <= 0 )
			Graph_screen_storage_max = 1;
	}
	else if ( strcmp( buff, "graph_screen_off_side_effect" ) == 0 )
	{
		encode_se( string,
			   F_graph_screen_off_side_effect,
			   GRAPH_SCREEN_SE_MAX );
	}
	else if ( strcmp( buff, "graph_screen_on_side_effect" ) == 0 )
	{
		encode_se( string,
			   F_graph_screen_on_side_effect,
			   GRAPH_SCREEN_SE_MAX );
	}
	else
	{
		return( 0 );		/* no match */
	}
	return( 1 );
}
