/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: pages.c,v 70.1 92/03/09 15:46:02 ssa Exp $ */
/**************************************************************************
* pages.c
*	Handle terminals with multiple screen pages.
*	Remember which windows are in which pages,
*	which have been modified off screen,
*	and which have been least recently used for taking over a new one.
**************************************************************************/
#include <stdio.h>
#include "ftwindow.h"
#include "ftterm.h"

#include "features.h"
int	F_pages = 1;

#include "pagingopt.h"
#include "winactive.h"

int	M_current_page_number = 0;

#define MAX_PAGES 10
int	Page[ MAX_PAGES ] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
int	Lru[ MAX_PAGES ] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int	Lru_sequence = 1;
#define MAX_LRU 8192
#include "wins.h"
/**************************************************************************
* remember_page
*	Remember that the window "old_window" is on the current page.
**************************************************************************/
remember_page( old_window )		/* good full going off page */
	FT_WIN	*old_window;
{
	int	winno;

					/* do not remember if 1 page terminal */
	if ( F_pages < 2 )
		return;
	winno = old_window->number;
					/* do not remember if not open 
					   and not a page per window terminal */
	if ( ( Window_active[ winno ] == 0 ) && ( F_pages < Wins ) )
		return;
					/* do not remember window 
					   if terminal has a page per window
					   and the window is on the wrong page.
					   e.g. selected split and then
					   expanded to full.
					*/
	if ( ( F_pages >= Wins ) && ( winno != M_current_page_number ) )
		return;
	Page[ M_current_page_number ] = winno;
	Lru[ M_current_page_number ] = Lru_sequence++;
	if ( Lru_sequence > MAX_LRU )
		reset_lru();
					/* mark it not changed offscreen */
	old_window->in_terminal = 1;
	clear_row_changed( old_window );
	clear_col_changed( old_window );
}
/**************************************************************************
* reset_lru
*	The least recently used list has overflowed - reset it.
**************************************************************************/
reset_lru()
{
	int	i;
	int	j;
	int	reseqed[ MAX_PAGES ];
	int	oldpage;
	int	oldlru;

	Lru_sequence = 1;
	for ( i = 0; i < F_pages; i++ )
		reseqed[ i ] = 0;
	for ( i = 0; i < F_pages; i++ )
	{
		oldlru = MAX_LRU + 1;
		oldpage = 0;				/* in case */
		for ( j = 0; j < F_pages; j++ )
		{
			if ( reseqed[ j ] == 0 )
			{
				if ( Lru[ j ] < oldlru )
				{
					oldpage = j;
					oldlru = Lru[ j ];
				}
			}
		}
		Lru[ oldpage ] = Lru_sequence++;
		reseqed[ oldpage ] = 1;
	}
}
/**************************************************************************
* find_page
*	Find the window "new_window" in the terminals pages and select it.
*	Return 0 if found.
*	Return non-zero if not there.
**************************************************************************/
find_page( new_window )
	FT_WIN	*new_window;
{
	int	i;
	int	winno;

	if ( F_pages < 2 )
		return( 1 );
	winno = new_window->number;
	for ( i = 0; i < F_pages; i++ )
	{
		if ( winno == Page[ i ] )
		{
			t_sync_page_number( i );
					/* mark it not remembered */
			Page[ M_current_page_number ] = -1;
			return( 0 );			/* found */
		}
	}
	return( 1 );					/* not found */
}
/**************************************************************************
* get_page
*	Pick a terminal page to use for window "new_window"
*	and select it.
*	Clear the screen if it has some other page in it.
**************************************************************************/
get_page( new_window )
	FT_WIN	*new_window;
{
	int	i;
	int	pickpage;
	int	modpage;
	int	oldpage;
	int	oldlru;

	if ( F_pages < 2 )
		return;
					/* if terminal has a page per window
					   then use the page that corresponds
					   to the window number.
					*/
	if ( F_pages >= Wins )
	{
		pickpage = new_window->number;
	}
	else
	{
		if ( Page[ M_current_page_number ] == -1 )
			return;
		pickpage = -1;
		modpage = -1;
		oldlru = MAX_LRU + 1;
		oldpage = 0;
		for ( i = 0; i < F_pages; i++ )
		{
			if ( Lock_window_1 && ( Page[ i ] == 0 ) )
				continue;
			if ( Page[ i ] == -1 )
			{			/* page is idle */
				pickpage = i;
				break;
			}
			else if ( Wininfo[ Page[ i ] ]->in_terminal == 0 )
			{		/* page's window modified offscreen */
				modpage = i;
			}
			else if ( Lru[ i ] < oldlru )
			{		/* page is oldest */
				oldpage = i;
				oldlru = Lru[ i ];
			}
		}
					/* use an idle page 
					   or one that was modified 
					   or the oldest
					*/
		if ( pickpage == -1 )
		{
			if ( modpage >= 0 )
				pickpage = modpage;
			else
				pickpage = oldpage;
		}
	}
	if ( pickpage != M_current_page_number )
	{
		t_sync_page_number( pickpage );
		Page[ M_current_page_number ] = -1;
		out_term_clear_screen();
	}
}
/**************************************************************************
* forget_page
*	Forget that window "new_window" is in the terminal memory.
**************************************************************************/
forget_page( new_window )		/* bringing window on split */
	FT_WIN	*new_window;
{
	int	i;
	int	winno;

	if ( F_pages < 2 )
		return;
	winno = new_window->number;
	for ( i = 0; i < F_pages; i++ )
	{
		if ( winno == Page[ i ] )
		{
			Page[ i ] = -1;
			new_window->in_terminal = 0;
			return;
		}
	}
}
/**************************************************************************
* forget_all_pages
*	Forget that any of the windows are in the terminal memory.
**************************************************************************/
forget_all_pages()
{
	int	i;

	for ( i = 0; i < F_pages; i++ )
		Page[ i ] = -1;
}
/**************************************************************************
* t_sync_page_number
*	Syncronize the terminal, if necessary, to the page number "pageno".
**************************************************************************/
t_sync_page_number( pageno )
	int	pageno;
{
	if ( pageno != M_current_page_number )
		term_display_page_number( pageno );
}
/**************************************************************************
* t_sync_page_number_normal
*	Syncronize the terminal, if necessary, to the page number 0.
**************************************************************************/
t_sync_page_number_normal()
{
	if ( M_current_page_number != 0 )
		term_display_page_number( 0 );
}
/**************************************************************************
* term_se_switch_page_number_0
*	An event has occurred that had the side effect of setting the
*	terminal to page 0.
**************************************************************************/
term_se_switch_page_number_0()
{
	int	page_number;

	page_number = M_current_page_number;
	M_current_page_number = 0;
	t_sync_page_number( page_number );
}
char	*T_display_page_number		= NULL;
char	*T_parm_display_page_next	= NULL;
char	*T_parm_display_page_prev	= NULL;
char	*T_display_page_next		= NULL;
char	*T_display_page_prev		= NULL;
extern char	*my_tparm();
/**************************************************************************
* term_display_page_number
*	Cause the terminal to select page number "page_number".
**************************************************************************/
term_display_page_number( page_number)
	int	page_number;
{
	char	*p;
	int	pages;
	int	parm[ 2 ];			/* 1 used - must be >= 2 - %i */
	char	*string_parm[ 1 ];		/* not used */

	if ( T_display_page_number != NULL )
	{
		parm[ 0 ] = page_number;
		p = my_tparm( T_display_page_number, parm, string_parm, -1 );
		my_tputs( p, 1 );
	}
	else if (  ( T_parm_display_page_prev != NULL )
		&& ( T_parm_display_page_next != NULL ) )
	{
		if ( page_number > M_current_page_number )
		{
			pages = page_number - M_current_page_number;
			parm[ 0 ] = pages;
			p = my_tparm( T_parm_display_page_next, parm,
							string_parm, -1 );
			my_tputs( p, 1 );
		}
		else
		{
			pages = M_current_page_number - page_number;
			parm[ 0 ] = pages;
			p = my_tparm( T_parm_display_page_prev, parm,
							string_parm, -1 );
			my_tputs( p, 1 );
		}
	}
	else if ( T_display_page_prev != NULL && T_display_page_next != NULL )
	{
		if ( page_number > M_current_page_number )
		{
		    for ( pages = M_current_page_number; pages < page_number;
							 pages++ )
			my_tputs( T_display_page_next, 1 );
		}
		else
		{
		    for ( pages = M_current_page_number; pages > page_number;
							 pages-- )
			my_tputs( T_display_page_prev, 1 );
		}
	}
	else
	{
		my_tputs( " PAGE ", 1 );
		return;
	}
	M_current_page_number = page_number;
}


/**************************************************************************
* get_environment_pages
*	Allow the user to override the number of pages of terminal memory
*	that are present.
**************************************************************************/
get_environment_pages()
{
	char	*p;
	int	pages;
	char	*getenv();

	p = getenv( "FACETPAGES" );
	if ( p != NULL )
	{
		pages = atoi( p );
		if ( pages >= 1 && pages <= MAX_PAGES )
		{
			F_pages = pages;
		}
		return;
	}
	p = getenv( "TSMPAGES" );
	if ( p != NULL )
	{
		pages = atoi( p );
		if ( pages >= 1 && pages <= MAX_PAGES )
		{
			F_pages = pages;
		}
	}
}
char *dec_encode();
/**************************************************************************
* extra_pages
*	TERMINAL DESCRIPTION PARSER module for '...page...'.
**************************************************************************/
/*ARGSUSED*/
extra_pages( buff, string, attr_on_string, attr_off_string ) 
	char	*buff;
	char	*string;
	char	*attr_on_string;		/* not used */
	char	*attr_off_string;		/* not used */
{
	if ( strcmp( buff, "pages" ) == 0 )
	{
		F_pages = atoi( string );
		if ( F_pages < 1 || F_pages > MAX_PAGES )
		{
			printf( "Invalid number of pages = %d\n", F_pages );
			F_pages = 1;
		}
	}
	else if ( strcmp( buff, "display_page_number" ) == 0 )
	{
		T_display_page_number = dec_encode( string );
	}
	else if ( strcmp( buff, "parm_display_page_next" ) == 0 )
	{
		T_parm_display_page_next = dec_encode( string );
	}
	else if ( strcmp( buff, "parm_display_page_prev" ) == 0 )
	{
		T_parm_display_page_prev = dec_encode( string );
	}
	else if ( strcmp( buff, "display_page_next" ) == 0 )
	{
		T_display_page_next = dec_encode( string );
	}
	else if ( strcmp( buff, "display_page_prev" ) == 0 )
	{
		T_display_page_prev = dec_encode( string );
	}
	else
	{
		return( 0 );		/* no match */
	}
	return( 1 );
}
