/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: perwindow.c,v 70.1 92/03/09 15:46:08 ssa Exp $ */
/***************************************************************************
*  perwindow.c
*  strings to be remembered for each window
*  syntax:
*	perwindow-length-flags-clears-cleared_by-order_group=string
*	perwindow_variable-length-flags-clears-cleared_by-order_group=string
*		length:	storage necessary to hold strings from this group.
*		flags:	
*			C = track Curwin only - not also ORW
*			L = remember last and do not output unnecessarily.
*			    may not be compatible with order_groups.
*			W = set to default on Window command mode - then curwin
*			R = set to default on Redo screen - then set to curwin
*			O = set to default on outwin output then set to curwin
*			S = set to default on Startup
*			I = set to default on window Idle
*			Q = set to default on Quit facetterm
*			F = tracks function keys
*			A = tracks screen Attribute
*		clears:	a thru o
*			when this pattern is seen it clears the perwindow
*			groups marked with the corresponding letter in the
*			cleared_by field.
*			The 'o' clear group is used for side effect soft
*			reset clears.
*		cleared_by: a thru o
*		order_group: any character
*			all contiguous entries with same order_group letter
*			must be output in the same order as was done by the
*			application.
*	perwindow_also=pattern
*	perwindow_also=pattern
*		pattern: all strings matching belong to this perwindow group.
*	perwindow_default=string
*	perwindow_pad=pad_string
*	perwindow_before=before_string
*	perwindow_after=after_string
*		pad_string: must be output after the perwindow string.
*Note:
*	L and not S (SYNC_MODE_START) implies assume last_output = LAST_DEFAULT
*******************************************************************************/
#include <stdio.h>
#include "ftproc.h"
#include "ftwindow.h"
#include "features.h"
#include "decode.h"
#include "wins.h"
#include "max_buff.h"

extern int errno;

typedef struct
{
	int	perwindowno;
	int	in_progress[ TOPWIN ];
	int	length[ TOPWIN ];
	int	finish_match[ TOPWIN ];
	char	*finish_string;
	int	invalid[ TOPWIN ];
} PERWINDOW_SPECIAL;
typedef struct
{
	short	length;
	short	clears;
	short	cleared_by;
	char	order_group;
	char	order[ TOPWIN ];
	char	*default_string;
	char	*window_string[ TOPWIN ];
	char	*last_ftkey_window_string[ TOPWIN ];
	char	*pad;
	char	*before_out;
	char	*after_out;
	char	*variable_out;
	short	last_output;
	short	sync_mode;
	PERWINDOW_SPECIAL	*perwindow_special;
	long	*side_effect;
} PERWINDOW;

#include "perwindow.h"

#define LAST_NOT_USED	-3
#define LAST_UNKNOWN	-2
#define LAST_DEFAULT	-1

#define MAX_PERWINDOW	500

int		T_perwindowno = 0;
PERWINDOW	*T_perwindow[ MAX_PERWINDOW ]={NULL};

#include "ftkey.h"
int		Processing_ftkey = 0;

/**************************************************************************
* fct_perwindow_special
**************************************************************************/
perwindow_special_char( s, c )
	PERWINDOW_SPECIAL	*s;
	unsigned char		c;
{
	PERWINDOW	*p;
	char		*w;
	char		msg_buff[ 80 ];
	int		winno;
	int		len;
	int		perwindowno;

	winno = Outwin->number;
	perwindowno = s->perwindowno;
	p = T_perwindow[ perwindowno ];
	w = p->window_string[ winno ];
	if ( ( w != NULL ) && ( s->invalid[ winno ] == 0 ) )
	{
		len = s->length[ winno ];
		if ( len >= p->length )
		{
			sprintf( msg_buff,
				"Perwindow_special#=%d-length=%d-max=%d",
				 perwindowno, len, p->length );
			error_record_msg( msg_buff );
			term_beep();
			s->invalid[ winno ] = 1;
		}
		else
		{
			w[ len ] = c;
			w[ len + 1 ] = '\0';
			s->length[ winno ]++;
		}
	}
	if ( c == s->finish_string[ s->finish_match[ winno ] ] )
	{
		s->finish_match[ winno ]++;
		if ( s->finish_match[ winno ] >= strlen( s->finish_string ) )
		{
			if ( ( w != NULL ) && ( s->invalid[ winno ] == 0 ) )
			{
						/* if this string clears other
						   perwindows, clear them out
						*/
				if ( p->clears )
					perwindow_clear_winno( p->clears,
							       winno );
						/* if order is important,
						   remember order 
						*/
				if ( p->order_group )
					set_order( perwindowno, winno );
						/* output if current window, or
						   marked for output if onscreen
						*/
				if (    outwin_is_curwin()
				   || (  Outwin->onscreen
				      && ( p->sync_mode & SYNC_MODE_OUTWIN )
				      )
				   || ( p->sync_mode & SYNC_MODE_PASS )
				   )
				{
					/**********************************
					* If perwindow has a side effect,
					* do it first.
					**********************************/
					if ( p->side_effect != (long *) 0 )
						term_se( p->side_effect );
					if ( p->before_out != (char *) 0 )
						term_write( p->before_out,
						    strlen( p->before_out ) );
					term_write( w, strlen( w ) );
					term_pad( p->pad );
					if ( p->after_out != (char *) 0 )
						term_write( p->after_out,
						    strlen( p->after_out ) );
					if ( p->last_output != LAST_NOT_USED )
					{
						p->last_output = winno;
					}
						/* if this string clears other
						   perwindows, clear them from
						   the last output remembered
						   in the terminal
						*/
					if ( p->clears )
					    perwindow_clear_last( p->clears );
				}
			}
			s->in_progress[ winno ] = 0;
			Outwin->special_function = NULL;
			Outwin->special_ptr = (long *) 0;
		}
	}
	else
		s->finish_match[ winno ] = 0;
}
/**************************************************************************
* fct_perwindow
*	ACTION module for 'fct_perwindow'.
*	Perwindow specification number "perwindowno" was matched with
*	the string "buff" and is "len" characters long.
**************************************************************************/
fct_perwindow( perwindowno, inbuff, inbufflen, string_parm_1 )
	int	perwindowno;
	char	*inbuff;			/* not null terminated */
	int	inbufflen;
	char	*string_parm_1;
{
	PERWINDOW	*p;
	char		*w;
	int		winno;
	char		msg_buff[ 80 ];
	int		storeok;
	char		*lw;
	int		len;
	char		*buff;
	char		variable_buff[ MAX_BUFF + 1 ];
	long		*malloc_run();

	p = T_perwindow[ perwindowno ];
	/******************************************************************
	* If perwindow has a side effect, do it first.
	******************************************************************/
	if ( p->side_effect != (long *) 0 )
	{
		win_se( p->side_effect );
	}
	/******************************************************************
	******************************************************************/
	if ( p->variable_out == NULL )
	{
		buff = inbuff;
		len = inbufflen;
	}
	else
	{
		variable_buff[ 0 ] = 'X';
		strncpy( &variable_buff[ 1 ], string_parm_1, MAX_BUFF - 1 );
		variable_buff[ MAX_BUFF ] = '\0';
		buff = variable_buff;
		len = strlen( buff );
	}
	winno = Outwin->number;
						/* get or create pointer to
						   storage for window */
	w = p->window_string[ winno ];
	if ( w == NULL )
	{
		w = (char *) malloc_run( p->length + 1, "perwindow_string" );
		if ( w == NULL )
		{
			printf( "ERROR: Perwindow malloc failed - %d\r\n",
					errno );
			term_outgo();
		}
		else
		{
			p->window_string[ winno ] = w;
			w[ 0 ] = '\0';
			w[ p->length ] = '\0';
		}
	}
						/* store if it will fit and
						   storage exists */
	if ( len > p->length )
	{
		sprintf( msg_buff, "Perwindow_#=%d-length=%d-max=%d",
			 perwindowno, len, p->length );
		error_record_msg( msg_buff );
		term_beep();
		storeok = 0;
	}
	else if ( w != NULL )
	{
		
		strncpy( w, buff, len );
		w[ len ] = '\0';
		storeok = 1;
	}
	else
		storeok = 0;
	/******************************************************************
	* 
	******************************************************************/
	if ( p->perwindow_special != (PERWINDOW_SPECIAL *) 0 )
	{
		PERWINDOW_SPECIAL	*s;

		s = p->perwindow_special;
		s->in_progress[ winno ] = 1;
		s->finish_match[ winno ] = 0;
		if ( storeok )
		{
			s->invalid[ winno ] = 0;
			s->length[ winno ] = strlen( w );
		}
		else
		{
			s->invalid[ winno ] = 1;
			s->length[ winno ] = 0;
		}
		Outwin->special_function = perwindow_special_char;
		Outwin->special_ptr = (long *) s;
		if ( p->last_output == winno )
			p->last_output = LAST_UNKNOWN;
		return;
	}
						/* if this string clears other
						   perwindows, clear them out
						*/
	if ( p->clears )
		perwindow_clear_winno( p->clears, winno );
						/* if order is important,
						   remember order 
						*/
	if ( p->order_group )
		set_order( perwindowno, winno );
						/* output if current window, or
						   marked for output if onscreen
						*/
	if (    outwin_is_curwin()
	   || ( Outwin->onscreen && ( p->sync_mode & SYNC_MODE_OUTWIN ) )
	   || ( p->sync_mode & SYNC_MODE_PASS )
	   )
	{
		/**************************************************
		* If perwindow has a side effect, do it first.
		**************************************************/
		if ( p->side_effect != (long *) 0 )
			term_se( p->side_effect );
		if ( p->before_out != (char *) 0 )
			term_write( p->before_out, strlen( p->before_out ) );
		term_write( inbuff, inbufflen );
		term_pad( p->pad );
		if ( p->after_out != (char *) 0 )
			term_write( p->after_out, strlen( p->after_out ) );
		if ( p->last_output != LAST_NOT_USED )
		{
			if ( storeok )
				p->last_output = winno;
			else
				p->last_output = LAST_UNKNOWN;
		}
						/* if this string clears other
						   perwindows, clear them from
						   the last output remembered
						   in the terminal
						*/
		if ( p->clears )
			perwindow_clear_last( p->clears );
	}
	if (  Processing_ftkey 			/* ^Wk in progress */
	   && ( p->sync_mode & SYNC_MODE_FUNCTION )
	   && storeok )
	{
						/* get or create pointer to
						   storage for last ftkey */
	    lw = p->last_ftkey_window_string[ winno ];
	    if ( lw == NULL )
	    {
		lw = (char *) malloc_run( p->length + 1,
					  "perwindow_last_ftkey" );
		if ( lw == NULL )
		{
			printf( 
			"ERROR: Perwindow last ftkey malloc failed - %d\r\n",
					errno );
			storeok = 0;
			term_outgo();
		}
		else
		{
			p->last_ftkey_window_string[ winno ] = lw;
			lw[ 0 ] = '\0';
			lw[ p->length ] = '\0';
		}
	    }
	    if ( storeok )
	    {
		strncpy( lw, buff, len );
		lw[ len ] = '\0';
	    }
	}
}
/**************************************************************************
* t_sync_perwindow_all
*	Syncronize the terminal to the perwindow capabilities of the window
*	"Outwin" that are appropriate for "sync_mode".
**************************************************************************/
t_sync_perwindow_all( sync_mode )
	int		sync_mode;
{
	int		winno;
	int		perwindowno;
	PERWINDOW	*p;
	int		order_group_index[ MAX_PERWINDOW ];
	int		num_order;
	int		i;

	winno = Outwin->number;
	perwindowno = 0;
	while ( perwindowno < T_perwindowno )
	{
		p = T_perwindow[ perwindowno ];
		if ( p->order_group )
		{
			num_order =  get_order( p->order_group, perwindowno,
						winno, order_group_index );
			for ( i = 0; i < num_order; i++ )
			{
				p = T_perwindow[ order_group_index[ i ] ];
				if ( p->sync_mode & sync_mode )
					term_sync_perwindow( p, winno );
			}
			perwindowno += num_order;
		}
		else if ( p->sync_mode & sync_mode )
		{
			term_sync_perwindow( p, winno );
			perwindowno++;
		}
		else
			perwindowno++;
	}
}
/**************************************************************************
* t_sync_perwindow_default
*	Syncronize the terminal to the default perwindow capabilities of the 
*	window "Outwin" that are appropriate for "sync_mode".
**************************************************************************/
t_sync_perwindow_default( sync_mode )
	int		sync_mode;
{
	int		perwindowno;
	PERWINDOW	*p;

	perwindowno = 0;
	while ( perwindowno < T_perwindowno )
	{
		p = T_perwindow[ perwindowno ];
		if ( p->sync_mode & sync_mode )
			term_sync_perwindow_default( p );
		perwindowno++;
	}
}
/**************************************************************************
* perwindow_init
*	Clear the remembered output for window "winno" for all perwindow
*	blocks that are marked "sync_mode".
*	E.G. clear all perwindows marked I - set to default on idle
**************************************************************************/
perwindow_init( sync_mode, winno, processing_ftkey )
	int		sync_mode;
	int		winno;
	int		processing_ftkey;	/* ^Wk in progress */
{
	int		perwindowno;
	PERWINDOW	*p;
	char		*w;
	char		*lw;

	perwindowno = 0;
	while ( perwindowno < T_perwindowno )
	{
		p = T_perwindow[ perwindowno ];
		if ( p->sync_mode & sync_mode )
		{
			w = p->window_string[ winno ];
			/**************************************************
			* if it had contents - get rid of it.
			**************************************************/
			if ( w != NULL )
				w[ 0 ] = '\0';
			/**************************************************
			**  If last output was this one, we just changed 
			**  it without changing terminal.
			**************************************************/
			if ( p->last_output == winno )
				p->last_output = LAST_UNKNOWN;
			/**************************************************
			** If ^WkDEFAULT - erase last ftkey setting.
			**************************************************/
			if ( processing_ftkey )
			{
				lw = p->last_ftkey_window_string[ winno ];
				if ( lw != NULL )
				{
					*lw = '\0';
				}
			}
		}
		perwindowno++;
	}
}
/**************************************************************************
* perwindow_ftkey_last
*	Set the perwindow strings for "winno" to the remembered strings
*	as the last loaded from a function key file.
*	Only the perwindow's marked with "sync_mode" are affected.
*	This is called when  ^W k +  is executed.
**************************************************************************/
perwindow_ftkey_last( sync_mode, winno )
	int		sync_mode;
	int		winno;
{
	int		perwindowno;
	PERWINDOW	*p;
	char		*w;
	char		*lw;
	int		changed;

	for ( perwindowno = 0; perwindowno < T_perwindowno; perwindowno++ )
	{
		p = T_perwindow[ perwindowno ];
		if ( p->sync_mode & sync_mode )
		{
			changed = 0;
			w = p->window_string[ winno ];
			if ( w == NULL )
				continue;
			lw = p->last_ftkey_window_string[ winno ];
			if ( lw != NULL )
			{
				if ( strcmp( w, lw ) != 0 )
				{
					strcpy( w, lw );
					changed = 1;
				}
			}
			else
			{
				if ( w[ 0 ] != '\0' )
				{
					w[ 0 ] = '\0';
					changed = 1;
				}
			}
			if ( changed )
			{
						/* if this string clears other
						   perwindows, clear them out
						*/
				if ( p->clears )
					perwindow_clear_winno( p->clears, 
								winno );
						/* if order is important,
						   remember order 
						*/
				if ( p->order_group )
					set_order( perwindowno, winno );
			}
			/**************************************************
			**  If last output was this one, we just changed 
			**  it without changing terminal.
			**************************************************/
			if ( ( p->last_output == winno ) && changed )
				p->last_output = LAST_UNKNOWN;
		}
	}
}
/**************************************************************************
* term_sync_perwindow
*	Output the string appropriate for window number "winno"
*	of the perwindow capability "p" if necessary.
**************************************************************************/
term_sync_perwindow( p, winno )
	PERWINDOW	*p;
	int		winno;
{
	char		*window_string;
	char		*default_string;
	char		*last_string;
	int		last_output;
	PERWINDOW_SPECIAL	*s;

						/* determine string last
						   output */
	if ( p->last_output >= 0 )
		last_string = p->window_string[ p->last_output ];
	else if ( p->last_output == LAST_DEFAULT )
		last_string = p->default_string;
	else 
		last_string = NULL;
						/* determine string to be 
						   output */
	if ( (s = p->perwindow_special) != (PERWINDOW_SPECIAL *) 0 )
		if ( s->in_progress[ winno ] )
			return;
	window_string = p->window_string[ winno ];
	default_string = p->default_string;
	if (  (  window_string != NULL ) && ( *window_string !='\0' ) )
	{
		if (  ( last_string == NULL ) 
		   || ( strcmp( last_string, window_string ) != 0 ) )
		{
			term_perwindow_out( p, window_string );
		}
		last_output = winno;
	}
	else if (  (  default_string != NULL ) && ( *default_string != '\0' ) )
	{
		if (  ( last_string == NULL ) 
		   || ( strcmp( last_string, default_string ) != 0 ) )
		{
			term_perwindow_out( p, default_string );
		}
		last_output = LAST_DEFAULT;
	}
	else
	{
		last_output = LAST_UNKNOWN;
	}
	if ( p->last_output != LAST_NOT_USED )
		p->last_output = last_output;
}
term_perwindow_out( p, string )
	PERWINDOW	*p;
	char		*string;
{
	char	*my_tparm();
	char	*out;
	int	parm[ 2 ];			/* 1 used - must be >= 2 - %i */
	char	*string_parm[ 1 ];		/* not used */

	/**************************************************
	* If perwindow has a side effect, do it first.
	**************************************************/
	if ( p->side_effect != (long *) 0 )
		term_se( p->side_effect );
	if ( p->before_out != (char *) 0 )
		term_write( p->before_out, strlen( p->before_out ) );
	if ( p->variable_out != NULL )
	{
		string_parm[ 0 ] = &string[ 1 ];
		out = my_tparm( p->variable_out, parm, string_parm, -1 );
		term_puts( out );
	}
	else
	{
		term_puts( string );
	}
	term_pad( p->pad );
	if ( p->after_out != (char *) 0 )
		term_write( p->after_out, strlen( p->after_out ) );
}
/**************************************************************************
* term_sync_perwindow_default
*	Syncronize the terminal, if necessary, to the perwindow default
*	for the perwindow capability "p".
**************************************************************************/
term_sync_perwindow_default( p )
	PERWINDOW	*p;
{
	char		*default_string;
	char		*last_string;
	int		last_output;

						/* determine string last
						   output */
	if ( p->last_output >= 0 )
		last_string = p->window_string[ p->last_output ];
	else if ( p->last_output == LAST_DEFAULT )
		last_string = p->default_string;
	else 
		last_string = NULL;
						/* determine string to be
						   output */
	default_string = p->default_string;
	if (  (  default_string != NULL ) && ( *default_string != '\0' ) )
	{
		if (  ( last_string == NULL ) 
		   || ( strcmp( last_string, default_string ) != 0 ) )
		{
			term_perwindow_out( p, default_string );
		}
		last_output = LAST_DEFAULT;
	}
	else
		last_output = LAST_UNKNOWN;
	if ( p->last_output != LAST_NOT_USED )
		p->last_output = last_output;
}
/**************************************************************************
* win_se_perwindow_clear_string
*	Soft reset on window "winno" clears any marked 
*	cleared_by "cleared_by_char" in the string "cleared_by_char_string".
**************************************************************************/
win_se_perwindow_clear_string( cleared_by_char_string, winno )
	char	*cleared_by_char_string;
	int	winno;
{
	char	*s;

	for ( s = cleared_by_char_string; *s != '\0'; s++ )
		win_se_perwindow_clear( (int) *s, winno );
}

/**************************************************************************
* win_se_perwindow_clear
*	Soft reset on window "winno" clears any marked 
*	cleared_by "cleared_by_char".
**************************************************************************/
win_se_perwindow_clear( cleared_by_char, winno )
	int	cleared_by_char;
	int	winno;
{
	int	index;
	int	clears;

	index = cleared_by_char - 'a';
	clears = ( 1 << index );
	perwindow_clear_winno( clears, winno );
}
/**************************************************************************
* perwindow_clear_winno
*	Clear the perwindow string for window "winno" of any perwindow node
*	that is marked cleared_by the type "clears".
**************************************************************************/
perwindow_clear_winno( clears, winno )
	short	clears;
	int	winno;
{
	int		perwindowno;
	PERWINDOW	*p;
	char		*w;

	for ( perwindowno = 0; perwindowno < T_perwindowno; perwindowno++ )
	{
		p = T_perwindow[ perwindowno ];
		if ( p->cleared_by & clears )
		{
			w = p->window_string[ winno ];
			if ( w != NULL )
				*w = '\0';
			if ( p->last_output == winno )
				p->last_output = LAST_UNKNOWN;
		}
	}
}
/**************************************************************************
* term_se_perwindow_clear
*	Soft reset on terminal clears any marked cleared_by "cleared_by_char".
**************************************************************************/
term_se_perwindow_clear( cleared_by_char )
	int	cleared_by_char;
{
	int	index;
	int	clears;

	index = cleared_by_char - 'a';
	clears = ( 1 << index );
	perwindow_clear_last( clears );
}
/**************************************************************************
* perwindow_clear_last
*	A string was output to the terminal that clears the group "clears".
*	Forget the last output for this group since it was busted.
**************************************************************************/
perwindow_clear_last( clears )
	short	clears;
{
	int		perwindowno;
	PERWINDOW	*p;

	for ( perwindowno = 0; perwindowno < T_perwindowno; perwindowno++ )
	{
		p = T_perwindow[ perwindowno ];
		if ( p->cleared_by & clears )
		{
			if ( p->last_output != LAST_NOT_USED )
				p->last_output = LAST_UNKNOWN;
		}
	}
}
/**************************************************************************
* term_se_perwindow_default_string
*	A sequence was output to the terminal defaults any marked
*	cleared_by "cleared_by_char" in the string "cleared_by_char_string".
**************************************************************************/
term_se_perwindow_default_string( cleared_by_char_string )
	char	*cleared_by_char_string;
{
	char	*s;

	for ( s = cleared_by_char_string; *s != '\0'; s++ )
		term_se_perwindow_default( (int) *s );
}
/**************************************************************************
* term_se_perwindow_default
*	A sequence was output to the terminal defaults any marked
*	cleared_by "cleared_by_char".
**************************************************************************/
term_se_perwindow_default( cleared_by_char )
	int	cleared_by_char;
{
	int	index;
	int	clears;

	index = cleared_by_char - 'a';
	clears = ( 1 << index );
	perwindow_default_last( clears );
}
/**************************************************************************
* perwindow_default_last
*	A string was output to the terminal that defaults the group "clears".
**************************************************************************/
perwindow_default_last( clears )
	short	clears;
{
	int		perwindowno;
	PERWINDOW	*p;

	for ( perwindowno = 0; perwindowno < T_perwindowno; perwindowno++ )
	{
		p = T_perwindow[ perwindowno ];
		if ( p->cleared_by & clears )
		{
			if ( p->last_output != LAST_NOT_USED )
				p->last_output = LAST_DEFAULT;
		}
	}
}
/**************************************************************************
* get_order
*	For the order group with the name "order_group",
*	whose first member has index "order_group_first"
*	sort the group for the window number "winno" and
*	return an array of indexes to them in "index".
*	Return the number of entries found.
*	This allows order dependent perwindows to be output in the
*	same order as was done by the application.
**************************************************************************/
get_order( order_group, order_group_first, winno, index )
	char	order_group;		/* order group name */
	int	order_group_first;	/* index in T_perwindow of 1st member */
	int	winno;
	int	index[];		/* perwindowno's in order */
{
	PERWINDOW	*p;
	int		num_order;		/* # entries this order group */
	int		order[ MAX_PERWINDOW ];
	int		i;
	int		switched;
	int		or;
	int		in;

	num_order = 0;
	for ( i = order_group_first; i < T_perwindowno; i++ )
	{
		p = T_perwindow[ i ];
		if ( p->order_group != order_group )
			break;
		index[ num_order ] = i;
		order[ num_order ] = p->order[ winno ];
		num_order++;
	}
	/******************************************************************
	* Sort the indexes into T_perwindow that are in the array index so that 
	* they are in order according to the array order
	******************************************************************/
	do
	{
		switched = 0;
		for ( i = 0; i < num_order - 1; i++ )
		{
			if ( order[ i ] > order[ i + 1 ] )
			{
				or = order[ i + 1 ];
				in = index[ i + 1 ];
				order[ i + 1 ] = order[ i ];
				index[ i + 1 ] = index[ i ];
				order[ i ] = or;
				index[ i ] = in;
				switched = 1;
			}
		}
	} while( switched );
	return( num_order );
}
/**************************************************************************
* set_order
*	The perwindow capability number "perwindowno" was just decoded
*	on "winno". Record it as the last output if in an order group.
**************************************************************************/
set_order( perwindowno, winno )
	int	perwindowno;
	int	winno;
{
	PERWINDOW	*p;
	PERWINDOW	*pp;
	int		first;
	int		last;
	int		i;
	char		order_group;
	int		order;
	int		old;
	int		max;

	p = T_perwindow[ perwindowno ];
	order_group = p->order_group;
	order = p->order[ winno ];
						/* find first member of group */
	for ( first = perwindowno; first >= 0; first-- )
	{
		pp = T_perwindow[ first ];
		if ( pp->order_group != order_group )
			break;
	}
	first++;
	/******************************************************************
	* Find last member of group.
	******************************************************************/
	for ( last = perwindowno; last < T_perwindowno; last++ )
	{
		pp = T_perwindow[ last ];
		if ( pp->order_group != order_group )
			break;
	}
	last--;
	/******************************************************************
	* Any that used to be after this one can be reduced 1 
	* if this one was not 0.
	******************************************************************/
	max = 0;
	for ( i = first; i <= last; i++ )
	{
		if ( i != perwindowno )
		{
			pp = T_perwindow[ i ];
			old = pp->order[ winno ];
			if ( ( order > 0 ) && ( old > order ) )
			{
				old--;
				pp->order[ winno ] = old;
			}
			if ( old > max )
				max = old;
		}
	}
						/* this one last and unique */
	p->order[ winno ] = max + 1;
}
/**************************************************************************
* t_reload_perwindow_all
*	Reload the terminal to the perwindow capabilities of the window
*	"Outwin" that are appropriate for "sync_mode".
**************************************************************************/
t_reload_perwindow_all( sync_mode )
	int		sync_mode;
{
	int		winno;
	int		perwindowno;
	PERWINDOW	*p;
	int		order_group_index[ MAX_PERWINDOW ];
	int		num_order;
	int		i;

	winno = Outwin->number;
	perwindowno = 0;
	while ( perwindowno < T_perwindowno )
	{
		p = T_perwindow[ perwindowno ];
		if ( p->order_group )
		{
			num_order =  get_order( p->order_group, perwindowno,
						winno, order_group_index );
			for ( i = 0; i < num_order; i++ )
			{
				p = T_perwindow[ order_group_index[ i ] ];
				if ( p->sync_mode & sync_mode )
					term_perwindow( p, winno );
			}
			perwindowno += num_order;
		}
		else if ( p->sync_mode & sync_mode )
		{
			term_perwindow( p, winno );
			perwindowno++;
		}
		else
			perwindowno++;
	}
}
/**************************************************************************
* term_perwindow
*	Output the string appropriate for window number "winno"
*	of the perwindow capability "p".
**************************************************************************/
term_perwindow( p, winno )
	PERWINDOW	*p;
	int		winno;
{
	char		*window_string;
	char		*default_string;
	int		last_output;
	PERWINDOW_SPECIAL	*s;

						/* determine string to be 
						   output */
	if ( (s = p->perwindow_special) != (PERWINDOW_SPECIAL *) 0 )
		if ( s->in_progress[ winno ] )
			return;
	window_string = p->window_string[ winno ];
	default_string = p->default_string;
	if (  (  window_string != NULL ) && ( *window_string !='\0' ) )
	{
		term_perwindow_out( p, window_string );
		last_output = winno;
	}
	else if (  (  default_string != NULL ) && ( *default_string != '\0' ) )
	{
		term_perwindow_out( p, default_string );
		last_output = LAST_DEFAULT;
	}
	else
	{
		last_output = LAST_UNKNOWN;
	}
	if ( p->last_output != LAST_NOT_USED )
		p->last_output = last_output;
}
/**************************************************************************
* init_windows_special
*	Initialize the special pointers of Outwin to no.
**************************************************************************/
init_windows_special()
{
	Outwin->special_function = NULL;
	Outwin->special_ptr = (long *) 0;
}
/**************************************************************************
* modes_init_special
*	Initialize the special pointers of "win" to no.
**************************************************************************/
modes_init_special( win )
	FT_WIN  *win;
{
	win->special_function = NULL;
	win->special_ptr = (long *) 0;
}
/****************************************************************************/

int	Valid_perwindowno = -1;
/**************************************************************************
* extra_perwindow
*	TERMINAL DESCRIPTION PARSER module for 'perwindow'.
**************************************************************************/
extra_perwindow( buff, string, attr_on_string, attr_off_string ) 
	char	*buff;
	char	*string;
	char	*attr_on_string;
	char	*attr_off_string;
{
	char		*dec_encode();
	char		*temp_encode();
	char		*encoded;
	PERWINDOW	*p;
	int		length;
	char		temp_storage[ MAX_BUFF + 1 ];
	int		winno;
	int		variable;
	long		*malloc_run();

	if (  ( strcmp( buff, "perwindow" ) == 0 )
	   || ( strcmp( buff, "perwindow_variable" ) == 0 ) )
	{
		if ( strcmp( buff, "perwindow_variable" ) == 0 )
			variable = 1;
		else
			variable = 0;
		if ( T_perwindowno < MAX_PERWINDOW )
		{
			Valid_perwindowno = -1;
			if ( attr_on_string == NULL )
			{
				printf( "perwindow has missing length\n" );
				return( 1 );
			}
			length = atoi( attr_on_string );
			if ( length <= 0 )
			{
				printf( "perwindow has invalid length '%s'\n",
					attr_on_string );
				return( 1 );
			}
			p = ( PERWINDOW * ) malloc_run( sizeof( PERWINDOW ),
							"perwindow_structure" );
			if ( p == NULL )
			{
				printf( "ERROR: Perwindow malloc failed - %d\n",
					errno );
				return( 1 );
			}
			else
			{
				if ( variable == 0 )
				{
				    encoded=temp_encode( string, temp_storage );
				    p->variable_out = NULL;
				}
				else
				{
				    encoded = dec_encode( string );
				    p->variable_out = encoded;
				}
				inst_perwindow( encoded, T_perwindowno );
				p->length = length;
				p->clears = 0;;
				p->cleared_by = 0;;
				p->order_group = 0;
				for ( winno = 0; winno < TOPWIN; winno++ )
					p->order[ winno ] = 0;
				p->default_string = NULL;
				for ( winno = 0; winno < TOPWIN; winno++ )
				{
					p->window_string[ winno ] = NULL;
					p->last_ftkey_window_string[ winno ] =
									NULL;
				}
				p->pad = NULL;
				p->before_out = (char *) 0;
				p->after_out =  (char *) 0;
				p->last_output = LAST_NOT_USED;
				p->sync_mode = 0;
				p->perwindow_special = (PERWINDOW_SPECIAL *) 0;
				p->side_effect = (long *) 0;
				perwindow_encode( p, attr_off_string );
				Valid_perwindowno = T_perwindowno;
				T_perwindow[ T_perwindowno ] = p;
				T_perwindowno++;
			}
		}
		else
		{
			printf( "Too many perwindow\n" );
		}
	}
	else if ( strcmp( buff, "perwindow_also" ) == 0 )
	{
		if ( Valid_perwindowno >= 0 )
		{
			p = T_perwindow[ Valid_perwindowno ];
			encoded=temp_encode( string, temp_storage );
			inst_perwindow( encoded, Valid_perwindowno );
		}
		else
		{
			printf( "perwindow_also ignored '%s'\n", string );
		}
	}
	else if ( strcmp( buff, "perwindow_default" ) == 0 )
	{
		if ( Valid_perwindowno >= 0 )
		{
			p = T_perwindow[ Valid_perwindowno ];
			if ( p->variable_out != NULL )
			{
				temp_storage[ 0 ] = 'X';
				strncpy( &temp_storage[ 1 ], string,
								MAX_BUFF - 1 );
				temp_storage[ MAX_BUFF ] = '\0';
				p->default_string = dec_encode( temp_storage );
			}
			else
				p->default_string = dec_encode( string );
		}
		else
		{
			printf( "perwindow_default ignored '%s'\n", string );
		}
	}
	else if ( strcmp( buff, "perwindow_pad" ) == 0 )
	{
		if ( Valid_perwindowno >= 0 )
		{
			p = T_perwindow[ Valid_perwindowno ];
			p->pad = dec_encode( string );
		}
		else
		{
			printf( "perwindow_pad ignored '%s'\n", string );
		}
	}
	else if ( strcmp( buff, "perwindow_before" ) == 0 )
	{
		if ( Valid_perwindowno >= 0 )
		{
			p = T_perwindow[ Valid_perwindowno ];
			p->before_out = dec_encode( string );
		}
		else
		{
			printf( "perwindow_before ignored '%s'\n", string );
		}
	}
	else if ( strcmp( buff, "perwindow_after" ) == 0 )
	{
		if ( Valid_perwindowno >= 0 )
		{
			p = T_perwindow[ Valid_perwindowno ];
			p->after_out = dec_encode( string );
		}
		else
		{
			printf( "perwindow_after ignored '%s'\n", string );
		}
	}
	else if ( strcmp( buff, "perwindow_special" ) == 0 )
	{
		PERWINDOW_SPECIAL	*s;

		if ( Valid_perwindowno < 0 )
		{
			printf( "perwindow_special ignored '%s'\n", string );
			return( 1 );
		}
		p = T_perwindow[ Valid_perwindowno ];
		s = ( PERWINDOW_SPECIAL * ) malloc_run( 
						sizeof( PERWINDOW_SPECIAL ),
						"perwindow_special" );
		if ( s == NULL )
		{
			printf( "ERROR: perwindow_special malloc failed - %d\n",
					errno );
			return( 1 );
		}
		s->perwindowno = Valid_perwindowno;
		for ( winno = 0; winno < TOPWIN; winno++ )
		{
			s->in_progress[ winno ] = 0;
			s->length[ winno ] = 0;
			s->finish_match[ winno ] = 0;
			s->invalid[ winno ] = 0;
		}
		s->finish_string = dec_encode( string );
		p->perwindow_special = s;
	}
	else if ( strcmp( buff, "perwindow_side_effect" ) == 0 )
	{
		long	*allocate_se();

		if ( Valid_perwindowno >= 0 )
		{
			p = T_perwindow[ Valid_perwindowno ];
			p->side_effect = allocate_se( string );
		}
		else
		{
			printf( "perwindow_side_effect ignored '%s'\n",
								string );
		}
	}
	else
	{
		return( 0 );		/* no match */
	}
	return( 1 );
}
/**************************************************************************
* perwindow_encode
*	Encode the string "string" that contains perwindow capabilities
*	described at the top of this module and store them
*	in the perwindow capability "p".
**************************************************************************/
perwindow_encode( p, string )
	PERWINDOW	*p;
	char		*string;
{
	char		*s;
	char		c;
	int		index;

	if ( string == NULL )
		return;
	s = string;
	c = *s++;
	while ( c != '\0' )
	{
		if ( c == '-' )
		{
			c = *s++;
			break;
		}
		if ( c == 'L' )
			p->last_output = LAST_UNKNOWN;
		else if ( c == 'S' )
			p->sync_mode |= SYNC_MODE_START;
		else if ( c == 'W' )
			p->sync_mode |= SYNC_MODE_WINDOW;
		else if ( c == 'O' )
			p->sync_mode |= SYNC_MODE_OUTWIN;
		else if ( c == 'R' )
			p->sync_mode |= SYNC_MODE_REDO_SCREEN;
		else if ( c == 'I' )
			p->sync_mode |= SYNC_MODE_IDLE;
		else if ( c == 'Q' )
			p->sync_mode |= SYNC_MODE_QUIT;
		else if ( c == 'C' )
			p->sync_mode |= SYNC_MODE_CURRENT;
		else if ( c == 'F' )
			p->sync_mode |= SYNC_MODE_FUNCTION;
		else if ( c == 'A' )
			p->sync_mode |= SYNC_MODE_SCREEN_ATTRIBUTE;
		else if ( c == 'P' )
			p->sync_mode |= SYNC_MODE_PASS;
		else
			printf( "Invalid perwindow sync_mode = '%c'\n", c );
		c = *s++;
	}
						/* If this is not flagged to
						   output the default value on 
						   startup,
						   then is is assumed to be at
						   the default value.
						   Fix last_output if it is 
						   being used.
						*/
	if (  ( ( p->sync_mode & SYNC_MODE_START ) == 0 )
	   && ( p->last_output != LAST_NOT_USED ) )
	{
		p->last_output = LAST_DEFAULT;
	}
						/* clears */
	while( c != '\0' )
	{
		if ( c == '-' )
		{
			c = *s++;
			break;
		}
		if ( ( c >= 'a' ) && ( c <= 'o' ) )
		{
			index = c - 'a';
			p->clears |= ( 1 << index );
		}
		else if ( ( c >= 'A' ) && ( c <= 'O' ) )
		{
			index = c - 'A';
			p->clears |= ( 1 << index );
		}
		else
			printf( "perwindow clears invalid char '%c'\n", c );
		c = *s++;
	}
						/* cleared_by */
	while( c != '\0' )
	{
		if ( c == '-' )
		{
			c = *s++;
			break;
		}
		if ( ( c >= 'a' ) && ( c <= 'o' ) )
		{
			index = c - 'a';
			p->cleared_by |= ( 1 << index );
		}
		else if ( ( c >= 'A' ) && ( c <= 'O' ) )
		{
			index = c - 'A';
			p->cleared_by |= ( 1 << index );
		}
		else
			printf( "perwindow cleared_by invalid char '%c'\n", c );
		c = *s++;
	}
						/* order group */
	if ( ( c != '\0' ) && ( c != '-' ) )
	{
		p->order_group = c;
	}
}
#ifdef lint
static int lint_alignment_warning_ok_1;
#endif
