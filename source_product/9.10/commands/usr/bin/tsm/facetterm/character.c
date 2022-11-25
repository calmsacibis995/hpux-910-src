/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: character.c,v 70.1 92/03/09 15:40:49 ssa Exp $ */
/**************************************************************************
* character.c
*	Character set handling, cursor,
**************************************************************************/
#include <stdio.h>
#include "characte_p.h"
#include "ftchar.h"
#include "ftproc.h"
#include "ftwindow.h"
#include "features.h"

int	M_cursor_on = 1;
char	*T_cursor_on = NULL;
char	*T_cursor_off = NULL;
int	F_cursor_type_turns_cursor_on = 0;


/**************************************************************************
* dec_cursor_on
*	DECODE module for 'cursor_on'.
**************************************************************************/
dec_cursor_on()
{
	fct_cursor_on();
}
/**************************************************************************
* inst_cursor_on
*	INSTALL module for 'cursor_on'.
**************************************************************************/
inst_cursor_on( str )
	char	*str;
{
	dec_install( "cursor_on", (UNCHAR *) str, dec_cursor_on, 0, 0,
	(char *) 0 );
}
/**************************************************************************
* fct_cursor_on
*	ACTION module for 'cursor_on'.
**************************************************************************/
fct_cursor_on()
{
	Outwin->cursor_on = 1;
	if ( Outwin->onscreen )
		term_cursor_on();
}
/**************************************************************************
* term_cursor_on
*	TERMINAL OUTPUT module for 'cursor_on'.
**************************************************************************/
term_cursor_on()
{
	if ( T_cursor_on != NULL )
	{
		term_tputs( T_cursor_on );
		M_cursor_on = 1;
	}
}
/**************************************************************************
* win_se_cursor
*	An operation has occurred that had the side effect of setting the
*	current output window's cursor to the state "cursor_on" ( 0=off )
**************************************************************************/
win_se_cursor( cursor_on )
	int	cursor_on;
{
	Outwin->cursor_on = cursor_on;
}
/**************************************************************************
* term_se_cursor
*	An operation has occurred that had the side effect of setting the
*	terminals's cursor to the state "cursor_on" ( 0=off )
**************************************************************************/
term_se_cursor( cursor_on )
	int	cursor_on;
{
	M_cursor_on = cursor_on;
}
/**************************************************************************
* dec_cursor_off
*	DECODE module for 'cursor_off'.
**************************************************************************/
dec_cursor_off()
{
	fct_cursor_off();
}
/**************************************************************************
* inst_cursor_off
*	INSTALL module for 'cursor_off'.
**************************************************************************/
inst_cursor_off( str )
	char	*str;
{
	dec_install( "cursor_off", (UNCHAR *) str, dec_cursor_off, 0, 0,
	(char *) 0 );
}
/**************************************************************************
* fct_cursor_off
*	ACTION module for 'cursor_off'.
**************************************************************************/
fct_cursor_off()
{
	Outwin->cursor_on = 0;
	if ( Outwin->onscreen )
		term_cursor_off();
}
/**************************************************************************
* term_cursor_off
*	TERMINAL OUTPUT module for 'cursor_off'.
**************************************************************************/
term_cursor_off()
{
	if ( T_cursor_off != NULL )
	{
		term_tputs( T_cursor_off );
		M_cursor_on = 0;
	}
}

int	M_cursor_type = 0;
#define MAX_CURSOR	10
int	T_cursor_typeno = 0;
char	*T_cursor_type[ MAX_CURSOR ]={NULL};

/**************************************************************************
* dec_cursor_type
*	DECODE module for 'cursor_type'.
*	E.G. blink steady block underline
**************************************************************************/
/*ARGSUSED*/
dec_cursor_type( typeno, parm_ptr )
	int	typeno;
	char	*parm_ptr;
{
	fct_cursor_type( typeno );
}
/**************************************************************************
* inst_cursor_type
*	INSTALL module for 'cursor_type'.
**************************************************************************/
inst_cursor_type( str, typeno )
	char	*str;
	int	typeno;
{
	dec_install( "cursor_type", (UNCHAR *) str, dec_cursor_type, 
		typeno, 0,
		(char *) 0 );
}
/**************************************************************************
* fct_cursor_type
*	ACTION module for 'cursor_type'.
**************************************************************************/
fct_cursor_type( typeno )
	int	typeno;
{
	Outwin->cursor_type = typeno;
	if ( F_cursor_type_turns_cursor_on )
		Outwin->cursor_on = 1;
	if ( Outwin->onscreen )
		term_cursor_type( typeno );
}
/**************************************************************************
* term_cursor_type
*	TERMINAL OUTPUT module for 'cursor_type'.
**************************************************************************/
term_cursor_type( typeno )
	int typeno;
{
	char	*cursorstr;

	if ( typeno < T_cursor_typeno )
	{
		cursorstr = T_cursor_type[ typeno ];
		if ( cursorstr != NULL )
		{
			term_tputs( cursorstr );
			M_cursor_type = typeno;
			if ( F_cursor_type_turns_cursor_on )
				M_cursor_on = 1;
		}
	}
}
/**************************************************************************
* t_sync_cursor
*	Syncronize the terminal (if necessary) to 
*	the cursor on/off state specified by "cursor_on" and
*	the cursor type specified by "cursor_type".
**************************************************************************/
t_sync_cursor( cursor_on, cursor_type )
	int	cursor_on;
	int	cursor_type;
{
	if ( cursor_type != M_cursor_type )
		term_cursor_type( cursor_type );
	if ( cursor_on != M_cursor_on )
	{
		if ( cursor_on )
			term_cursor_on();
		else
			term_cursor_off();
	}
}
/**************************************************************************/
#define MAX_CHARACTER_SET_SELECTIONS	20
int	M_select_character_set[ MAX_CHARACTER_SETS ] = {0};
int	sT_select_character_set_selno[ MAX_CHARACTER_SETS ] = {0};
char	*sT_select_character_set[ MAX_CHARACTER_SETS ]
			       [ MAX_CHARACTER_SET_SELECTIONS ]={NULL};
FTCHAR	sT_character_set_attribute[ MAX_CHARACTER_SETS ]
			       [ MAX_CHARACTER_SET_SELECTIONS ]={0};

int	M_character_set = 0;
FTCHAR	M_character_set_attribute = 0;
int	sF_select_character_set_noload = { 0 };
int	sF_select_character_set_sets_current = { 0 };
int	sF_sets_current_translation[ MAX_CHARACTER_SETS ]
				   [ MAX_CHARACTER_SET_SELECTIONS ] = { 0 };

/**************************************************************************
* dec_0_select_character_set
*	DECODE module for 'select_character_set_0'.
*	Set character set G0.
**************************************************************************/
/*ARGSUSED*/
dec_0_select_character_set( selection, parm_ptr )
	int	selection;
	char	*parm_ptr;
{
	fct_select_character_set( 0, selection );
}
/**************************************************************************
* dec_1_select_character_set
*	DECODE module for 'select_character_set_1'.
*	Set character set G1.
**************************************************************************/
/*ARGSUSED*/
dec_1_select_character_set( selection, parm_ptr )
	int	selection;
	char	*parm_ptr;
{
	fct_select_character_set( 1, selection );
}
/**************************************************************************
* dec_2_select_character_set
*	DECODE module for 'select_character_set_2'.
*	Set character set G2.
**************************************************************************/
/*ARGSUSED*/
dec_2_select_character_set( selection, parm_ptr )
	int	selection;
	char	*parm_ptr;
{
	fct_select_character_set( 2, selection );
}
/**************************************************************************
* dec_3_select_character_set
*	DECODE module for 'select_character_set_3'.
*	Set character set G3.
**************************************************************************/
/*ARGSUSED*/
dec_3_select_character_set( selection, parm_ptr )
	int	selection;
	char	*parm_ptr;
{
	fct_select_character_set( 3, selection );
}
/**************************************************************************
* inst_select_character_set
*	INSTALL module for 'select_character_set_x'.
**************************************************************************/
inst_select_character_set( str, set, selection )
	char	*str;
	int	selection;
{
	switch( set )
	{
	case 0:
		dec_install( "select_character_set_0", (UNCHAR *) str,
			dec_0_select_character_set, selection, 0,
			(char *) 0 );
		break;
	case 1:
		dec_install( "select_character_set_1", (UNCHAR *) str,
			dec_1_select_character_set, selection, 0,
			(char *) 0 );
		break;
	case 2:
		dec_install( "select_character_set_2", (UNCHAR *) str,
			dec_2_select_character_set, selection, 0,
			(char *) 0 );
		break;
	case 3:
		dec_install( "select_character_set_3", (UNCHAR *) str,
			dec_3_select_character_set, selection, 0,
			(char *) 0 );
		break;
	default:
		printf( "program error: select character set = %d\n", set );
		break;
	}
}
/**************************************************************************
* fct_select_character_set
*	ACTION module for 'select_character_set_x'.
**************************************************************************/
fct_select_character_set( set, selection )
	int	set;
	int	selection;
{
	if ( oF_select_character_set_sets_current )
	{
		set = Outwin->character_set;
		if ( set > 0 )
		{
			selection =
				oF_sets_current_translation[ set ][selection ];
		}
	}
	Outwin->select_character_set[ set ] = selection;
	if (  ( set == Outwin->character_set )
	   && ( oF_select_character_set_noload == 0 ) )
	{
		Outwin->ftattrs &= (~ F_character_set_attributes );
		Outwin->ftattrs |= 
				oT_character_set_attribute[ set ][ selection ];
	}
	if ( Outwin->onscreen )
		term_select_character_set( set, selection );
}
/**************************************************************************
* win_se_character_set_normal
*	An operation has occurred that has the side effect of setting the
*	current output window's character set and character set shift
*	state to the base condition.
**************************************************************************/
win_se_character_set_normal()
{
	int	i;

	/* fattrs set in win_se_lock_shift */
	for( i = 0; i < MAX_CHARACTER_SETS; i++ )
	{
		Outwin->select_character_set[ i ] = 0;
	}
	win_se_lock_shift( 0 );
}
/**************************************************************************
* term_select_character_set
*	TERMINAL OUTPUT module for 'select_character_set_x'.
**************************************************************************/
term_select_character_set( set, selection )
	int	set;
	int	selection;
{
	char	*selectionstr;

	if ( mF_select_character_set_sets_current )
	{
		if ( term_lock_shift( set ) == 0 )
			return;
	}
	if ( selection < mT_select_character_set_selno[ set ] )
	{
		selectionstr = mT_select_character_set[ set ][ selection ];
		if ( selectionstr != NULL )
		{
			term_tputs( selectionstr );
			M_select_character_set[ set ] = selection;
			if (  ( set == M_character_set ) 
			   && ( mF_select_character_set_noload == 0 ) )
			{
				M_character_set_attribute = 
				mT_character_set_attribute[ set ][ selection ];
			}
		}
	}
}
/**************************************************************************
* term_se_select_character_set
*	An operation has occurred that has the side effect of setting the
*	terminal's character set selection for the character set shift state
*	"set" to "selection.
**************************************************************************/
term_se_select_character_set( set, selection )
	int	set;
	int	selection;
{
	M_select_character_set[ set ] = selection;
	if (  ( set == M_character_set ) 
	   && ( mF_select_character_set_noload == 0 ) )
	{
		M_character_set_attribute = 
			mT_character_set_attribute[ set ][ selection ];
	}
}
/*************************************************************************/
		/**********************************************************
		* Strings to set the terminal to the various character set
		* shift states. Lock and single shift respectively.
		**********************************************************/
char	*sT_lock_shift[ MAX_CHARACTER_SETS ] = {NULL};
char	*sT_single_shift[ MAX_CHARACTER_SETS ] = {NULL};

/**************************************************************************
* dec_lock_shift
*	DECODE module for 'lock_shift_x'.
*	Set G0 G1 G2 G3 as the current character set.
**************************************************************************/
/*ARGSUSED*/
dec_lock_shift( set, parm_ptr )
	int	set;
	char	*parm_ptr;
{
	fct_lock_shift( set );
}
/**************************************************************************
* inst_lock_shift
*	INSTALL module for 'lock_shift_x'.
**************************************************************************/
inst_lock_shift( str, set )
	char	*str;
	int	set;
{
	switch( set )
	{
	case 0:
		dec_install( "lock_shift_0", (UNCHAR *) str,
			dec_lock_shift, set, 0,
			(char *) 0 );
		break;
	case 1:
		dec_install( "lock_shift_1", (UNCHAR *) str,
			dec_lock_shift, set, 0,
			(char *) 0 );
		break;
	case 2:
		dec_install( "lock_shift_2", (UNCHAR *) str,
			dec_lock_shift, set, 0,
			(char *) 0 );
		break;
	case 3:
		dec_install( "lock_shift_3", (UNCHAR *) str,
			dec_lock_shift, set, 0,
			(char *) 0 );
		break;
	default:
		printf( "program error: lock shift = %d\n", set );
		break;
	}
}
/**************************************************************************
* fct_lock_shift
*	ACTION module for 'lock_shift_x'.
**************************************************************************/
fct_lock_shift( set )
	int	set;
{
	Outwin->character_set = set;
	Outwin->ftattrs &= (~ F_character_set_attributes );
	Outwin->ftattrs |= oT_character_set_attribute[ set ]
					[ Outwin->select_character_set[ set ] ];
	if ( Outwin->onscreen )
		term_lock_shift( set );
}
/**************************************************************************
* win_se_lock_shift
*	An operation has occurred that had the side effect of setting
*	the current output window's shift state to "set".
**************************************************************************/
win_se_lock_shift( set )
	int	set;
{
	Outwin->character_set = set;
	Outwin->ftattrs &= (~ F_character_set_attributes );
	Outwin->ftattrs |= oT_character_set_attribute[ set ]
				[ Outwin->select_character_set[ set ] ];
}
/**************************************************************************
* term_lock_shift
*	TERMINAL OUTPUT module for 'lock_shift_x'.
**************************************************************************/
term_lock_shift( set )		/* 1 = did it */
	int	set;
{
	char	*lock_shiftstr;

	if ( set < MAX_CHARACTER_SETS )
	{
		lock_shiftstr = mT_lock_shift[ set ];
		if ( lock_shiftstr != NULL )
		{
			term_tputs( lock_shiftstr );
			M_character_set = set;
			M_character_set_attribute = 
				mT_character_set_attribute[ set]
					[ M_select_character_set[ set ] ];
			return( 1 );
		}
	}
	return( 0 );
}
/**************************************************************************
* term_se_lock_shift
*	An operation has occurred that had the side effect of setting
*	the terminals lock shift state to "set".
**************************************************************************/
term_se_lock_shift( set )
	int	set;
{
	M_character_set = set;
	M_character_set_attribute = 
	    mT_character_set_attribute[ set] [ M_select_character_set[ set ] ];
}
/**************************************************************************
* dec_single_shift
*	DECODE module for 'single_shift_x'.
*	Select G0 G1 G2 G3 for one character.
**************************************************************************/
/*ARGSUSED*/
dec_single_shift( set, parm_ptr, parms_valid, parm )
	int	set;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
{
	fct_single_shift( set, (UNCHAR) parm[ 0 ],
			oT_character_set_attribute[ set ]
				[ Outwin->select_character_set[ set ] ] );
}
/**************************************************************************
* inst_single_shift
*	INSTALL module for 'single_shift_x'.
**************************************************************************/
#include "decode.h"
inst_single_shift( str, set )
	char	*str;
	int	set;
{
	switch( set )
	{
	case 0:
		dec_install( "single_shift_0", (UNCHAR *) str,
			dec_single_shift, set, BYTE_PARM_FOLLOWS_OPTION,
			(char *) 0 );
		break;
	case 1:
		dec_install( "single_shift_1", (UNCHAR *) str,
			dec_single_shift, set, BYTE_PARM_FOLLOWS_OPTION,
			(char *) 0 );
		break;
	case 2:
		dec_install( "single_shift_2", (UNCHAR *) str,
			dec_single_shift, set, BYTE_PARM_FOLLOWS_OPTION,
			(char *) 0 );
		break;
	case 3:
		dec_install( "single_shift_3", (UNCHAR *) str,
			dec_single_shift, set, BYTE_PARM_FOLLOWS_OPTION,
			(char *) 0 );
		break;
	default:
		printf( "program error: single shift = %d\n", set );
		break;
	}
}
/**************************************************************************
* term_single_shift
*	TERMINAL OUTPUT module for 'single_shift_x'.
**************************************************************************/
term_single_shift( set )
	int	set;
{
	char	*single_shiftstr;

	if ( set < MAX_CHARACTER_SETS )
	{
		single_shiftstr = mT_single_shift[ set ];
		if ( single_shiftstr != NULL )
		{
			term_tputs( single_shiftstr );
			return( 1 );
		}
	}
	return( 0 );
}
/*************************************************************************/
/**************************************************************************
* t_sync_character_set_base
*	Syncronize the terminal if necessary to it's default character set
*	selections for each shift state and it's shift state.
**************************************************************************/
t_sync_character_set_base()
{
	int	i;

	for( i = 0; i < MAX_CHARACTER_SETS; i++ )
	{
		if ( M_select_character_set[ i ] != 0 )
			term_select_character_set( i, 0 );
	}
	if (  ( M_character_set != 0 ) 
	   || ( M_character_set_attribute != 0 ) )
	{
		term_lock_shift( 0 );
	}
}
/**************************************************************************
* term_se_character_set_normal
*	An operation has occurred that had the side effect of setting
*	the terminal's character set selections and shift state to its
*	default.
**************************************************************************/
term_se_character_set_normal()
{
	int	i;

	for( i = 0; i < MAX_CHARACTER_SETS; i++ )
		term_se_select_character_set( i, 0 );
	term_se_lock_shift( 0 );
}
/**************************************************************************
* t_sync_character_set
*	Syncronize the terminal, if necessary, to the character set selections
*	and shift state of the current output window.
**************************************************************************/
t_sync_character_set()
{
	int	i;
	int	selection;
	FTCHAR	ftattrs;
	int	set;
	char	buff[ 80 ];

	if ( mF_select_character_set_noload )
	{
		/* make terminal right */
		set = Outwin->character_set;
		ftattrs = Outwin->ftattrs & F_character_set_attributes;
		if (  ( set != M_character_set )
		   || ( ftattrs != M_character_set_attribute ) )
		{
			for( selection = 0;
			     selection < mT_select_character_set_selno[ set ];
			     selection++ )
			{
				if ( ftattrs == 
				mT_character_set_attribute[ set ][ selection ] )
				{
					term_select_character_set( set, 
								selection );
					break;
				}
			}
			if ( selection >= mT_select_character_set_selno[ set ] )
			{
				sprintf( buff, 
				     "t_sync_character_set-%lx-%d", 
				     ftattrs, set );
				error_record_msg( buff );
				term_beep();
			}
			term_lock_shift( set );
		}
		for( i = 0; i < MAX_CHARACTER_SETS; i++ )
		{
			selection = Outwin->select_character_set[ i ];
			if ( selection != M_select_character_set[ i ] )
				term_select_character_set( i, selection );
		}
		return;
	}
	for( i = 0; i < MAX_CHARACTER_SETS; i++ )
	{
		selection = Outwin->select_character_set[ i ];
		if ( selection != M_select_character_set[ i ] )
			term_select_character_set( i, selection );
	}
	if ( Outwin->character_set != M_character_set )
		term_lock_shift( (int) Outwin->character_set );
}
/**************************************************************************
* t_sync_character_set_to_attr
*	Syncronize the terminal, if necessary, to the character set
*	selection and/or shift state that matches the character set
*	attributes in the attribute "ftattrs".
**************************************************************************/
t_sync_character_set_to_attr( ftattrs )
	FTCHAR	ftattrs;
{
	int	set;
	int	selection;
	char	buff[ 80 ];

	ftattrs &= F_character_set_attributes;
	if ( mF_select_character_set_noload )
	{
		if ( ftattrs == M_character_set_attribute )
			return;
		for( set = 0; set < MAX_CHARACTER_SETS; set++ )
		{
			if ( mT_select_character_set_selno[ set ] == 0 )
				continue;
			selection = M_select_character_set[ set ];
			if ( ftattrs == 
				mT_character_set_attribute[ set ][ selection ] )
			{
				if ( term_lock_shift( set ) )
				{
					return;
				}
			}
		}
		for( set = 0; set < MAX_CHARACTER_SETS; set++ )
		{
			for(  selection = 0;
			      selection < mT_select_character_set_selno[ set ];
			      selection++ )
			{
				if ( ftattrs == 
				mT_character_set_attribute[ set ][ selection ] )
				{
					term_select_character_set( set, 
								selection );
					if ( term_lock_shift( set ) )
						return;
					else
					{
						term_beep();
						sprintf( buff, 
			"t_sync_character_set_to_attr-noload_lock-%lx-%d",
							ftattrs, set );
						error_record_msg( buff );
						term_beep();
					}
				}
			}
		}
		sprintf( buff, 
			"t_sync_character_set_to_attr-noload-%lx", ftattrs );
		error_record_msg( buff );
		term_beep();
		return;
	}
	if ( ftattrs == mT_character_set_attribute[ M_character_set]
				[ M_select_character_set[ M_character_set ] ] )
		return;
	for( set = 0; set < MAX_CHARACTER_SETS; set++ )
	{
		if ( mT_select_character_set_selno[ set ] == 0 )
			continue;
		selection = M_select_character_set[ set ];
		if ( ftattrs == mT_character_set_attribute[ set ][ selection ] )
		{
			if ( set != M_character_set )
			{
				if ( term_lock_shift( set ) )
				{
					return;
				}
			}
		}
	}
	for( set = 0; set < MAX_CHARACTER_SETS; set++ )
	{
		for(	selection = 0;
			selection < mT_select_character_set_selno[ set ]; 
			selection++ )
		{
			if ( ftattrs == 
			     mT_character_set_attribute[ set ][ selection ] )
			{
				term_select_character_set( set, selection );
				if ( set != M_character_set )
				{
					if ( term_lock_shift( set ) )
					{
					}
					else if ( term_single_shift( set ) )
					{
					}
					else
					{
						term_beep();
						sprintf( buff,
			"t_sync_character_set_to_attr-shift-%lx-%d",
							ftattrs, set );
						error_record_msg( buff );
						term_beep();
					}
				}
				return;
			}
		}
	}
	term_beep();
	sprintf( buff, "t_sync_character_set_to_attr-load-%lx", ftattrs );
	error_record_msg( buff );
	term_beep();
}
/**************************************************************************
* m_sync_character_set_to_attr
*	An operation has occurred that had the side effect of setting
*	the current output window to a character set
*	selection and/or shift state that matches the character set
*	attributes in the attribute "ftattrs".
*	Set the window to an appropriate character set selection and/or
*	shift state.
**************************************************************************/
m_sync_character_set_to_attr( ftattrs )	/* character_set change side effect */
	FTCHAR	ftattrs;
{
	int	set;
	int	selection;

	ftattrs &= F_character_set_attributes;
	set = Outwin->character_set;
	selection = Outwin->select_character_set[ set ];
	if ( ftattrs == oT_character_set_attribute[ set][ selection ] )
		return;
	for( set = 0; set < MAX_CHARACTER_SETS; set++ )
	{
		for(	selection = 0;
			selection < oT_select_character_set_selno[ set ]; 
			selection++ )
		{
			if ( ftattrs == 
			     oT_character_set_attribute[ set ][ selection ] )
			{
				Outwin->character_set = set;
				Outwin->select_character_set[ set ] =
								selection;
				return;
			}
		}
	}
	term_beep();
}
/****************************************************************************/
/**************************************************************************
* extra_character
*	TERMINAL DESCRIPTION PARSER module for
*		cursor_...
*		character_set...
**************************************************************************/
char *dec_encode();
extra_character( buff, string, attr_on_string, attr_off_string ) 
	char	*buff;
	char	*string;
	char	*attr_on_string;
	char	*attr_off_string;
{
	if ( strcmp( buff, "cursor_on" ) == 0 )
	{
		T_cursor_on = dec_encode( string );
		inst_cursor_on( T_cursor_on );
	}
	else if ( strcmp( buff, "cursor_off" ) == 0 )
	{
		T_cursor_off = dec_encode( string );
		inst_cursor_off( T_cursor_off );
	}
	else if ( strcmp( buff, "cursor_type" ) == 0 )
	{
		if ( T_cursor_typeno < MAX_CURSOR )
		{
			T_cursor_type[ T_cursor_typeno ] =
				dec_encode( string );
			inst_cursor_type(
				T_cursor_type[ T_cursor_typeno ],
				T_cursor_typeno );
			T_cursor_typeno++;
		}
		else
		{
			printf( "Too many cursor_type\n" );
		}
	}
	else if ( strcmp( buff, "pc_cursor_on" ) == 0 )
	{
	}
	else if ( strcmp( buff, "pc_cursor_off" ) == 0 )
	{
	}
	else if ( strcmp( buff, "pc_cursor_type" ) == 0 )
	{
	}
	else if ( strcmp( buff, "cursor_type_turns_cursor_on" ) == 0 )
	{
		F_cursor_type_turns_cursor_on =
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "select_character_set_sets_current" ) == 0 )
	{
		xF_select_character_set_sets_current = 1;
	}
	else if ( strcmp( buff, "select_character_set_0" ) == 0 )
	{
		test_select_character_set( 0, string,
					attr_on_string, attr_off_string );
	}
	else if ( strcmp( buff, "select_character_set_1" ) == 0 )
	{
		test_select_character_set( 1, string,
					attr_on_string, attr_off_string );
	}
	else if ( strcmp( buff, "select_character_set_2" ) == 0 )
	{
		test_select_character_set( 2, string,
					attr_on_string, attr_off_string );
	}
	else if ( strcmp( buff, "select_character_set_3" ) == 0 )
	{
		test_select_character_set( 3, string,
					attr_on_string, attr_off_string );
	}
	else if ( strcmp( buff, "select_character_set_noload" ) == 0 )
	{
		xF_select_character_set_noload = 1;
	}
	else if ( strcmp( buff, "lock_shift_0" ) == 0 )
	{
		test_lock_shift( 0, string );
	}
	else if ( strcmp( buff, "lock_shift_1" ) == 0 )
	{
		test_lock_shift( 1, string );
	}
	else if ( strcmp( buff, "lock_shift_2" ) == 0 )
	{
		test_lock_shift( 2, string );
	}
	else if ( strcmp( buff, "lock_shift_3" ) == 0 )
	{
		test_lock_shift( 3, string );
	}
	else if ( strcmp( buff, "single_shift_0" ) == 0 )
	{
		test_single_shift( 0, string );
	}
	else if ( strcmp( buff, "single_shift_1" ) == 0 )
	{
		test_single_shift( 1, string );
	}
	else if ( strcmp( buff, "single_shift_2" ) == 0 )
	{
		test_single_shift( 2, string );
	}
	else if ( strcmp( buff, "single_shift_3" ) == 0 )
	{
		test_single_shift( 3, string );
	}
	else
	{
		return( 0 );		/* no match */
	}
	return( 1 );
}
/**************************************************************************
* test_select_character_set
*	Encode and install a "select_character_set_X" terminal description.
**************************************************************************/
/*ARGSUSED*/
test_select_character_set( set, string, attr_on_string, attr_off_string )
	int	set;
	char	*string;
	char	*attr_on_string;
	char	*attr_off_string;		/* not used */
{
	char	*encoded;
	int	selection;
	FTCHAR	attribute;
	FTCHAR	attribute_encode();

	selection = xT_select_character_set_selno[ set ];
	if ( selection < MAX_CHARACTER_SET_SELECTIONS )
	{
		encoded = dec_encode( string );
		xT_select_character_set[ set ] [ selection ] = encoded;
		if ( encoded[ 0 ] != '\0' )
		{
		    if ( xF_select_character_set_sets_current && ( set > 0 ) )
		    {
			int	i;
			int	num;

			num = xT_select_character_set_selno[ 0 ];
			for ( i = 0; i < num; i++ )
			{
				if ( strcmp( encoded,
				     xT_select_character_set[ 0 ][ i ] ) == 0 )
				{
				    xF_sets_current_translation[ set ][ i ] =
								selection;
				    break;
				}
			}
			if ( i >= num )
			{
				printf( 
				 "No match to select_character_set_%d sel_%d\n",
				  set, selection );
			}
		    }
		    else
			inst_select_character_set( encoded, set, selection );
		}
		attribute = attribute_encode( attr_on_string );
		F_character_set_attributes |= attribute;
		xT_character_set_attribute[ set ] [ selection ] = attribute;
		xT_select_character_set_selno[ set ]++;
	}
	else
	{
		printf( "Too many select_character_set_%d\n", set );
	}
}
/**************************************************************************
* test_lock_shift
*	Encode and install a "lock_shift_X" terminal description.
**************************************************************************/
test_lock_shift( set, string )
	int	set;
	char	*string;
{
	char	*encoded;

	encoded = dec_encode( string );
	xT_lock_shift[ set ] = encoded;
	inst_lock_shift( encoded, set );
}
/**************************************************************************
* test_single_shift
*	Encode and install a "single_shift_X" terminal description.
**************************************************************************/
test_single_shift( set, string )
	int	set;
	char	*string;
{
	char	*encoded;

	encoded = dec_encode( string );
	xT_single_shift[ set ] = encoded;
	inst_single_shift( encoded, set );
}
