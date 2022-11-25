/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: attribute.c,v 70.1 92/03/09 15:40:25 ssa Exp $ */
/**************************************************************************
* Modules supporting attributes such as reverse video
**************************************************************************/
#include <stdio.h>
#include "attribut_p.h"
#include "ftchar.h"
#include "ftproc.h"
#include "ftwindow.h"
#include "options.h"

#include "myattrs.h"
#include "decode.h"

FTCHAR	M_attribute = 0;

#include "features.h"
int	sF_graphics_escape_control = { 0 };
int	sF_graphics_escape_delete = { 0 };
int	sF_delete_is_a_character = { 0 };

char	*sT_exit_attribute_mode = { NULL };
FTCHAR	sT_exit_attribute_mode_off;

char	*sT_prime_cva = { NULL };
FTCHAR	sT_prime_cva_off;

typedef struct t_attribute_struct T_ATTRIBUTE;
struct t_attribute_struct
{
	T_ATTRIBUTE		*next;
	char			*t_attribute;
	FTCHAR			t_attribute_on;
	FTCHAR			t_attribute_off;
	FTCHAR			t_attribute_on_ok;
	FTCHAR			t_attribute_off_ok;
};
T_ATTRIBUTE	*sT_attribute_ptr = { (T_ATTRIBUTE *) 0 };

typedef struct t_attribute_next_struct T_ATTRIBUTE_NEXT;
struct t_attribute_next_struct
{
	T_ATTRIBUTE_NEXT	*next;
	char			*t_attribute_next;
	FTCHAR			t_attribute_next_on;
	FTCHAR			t_attribute_next_off;
	FTCHAR			t_attribute_next_on_ok;
	FTCHAR			t_attribute_next_off_ok;
};
T_ATTRIBUTE_NEXT	*sT_attribute_next_ptr = { (T_ATTRIBUTE_NEXT *) 0 };

typedef struct t_parm_attribute_struct T_PARM_ATTRIBUTE;
struct t_parm_attribute_struct
{
	T_PARM_ATTRIBUTE	*next;
	FTCHAR			t_parm_attribute_on;
	FTCHAR			t_parm_attribute_off;
	char			*t_parm_attribute_implied_val;
};
T_PARM_ATTRIBUTE	*sT_parm_attribute_ptr = { (T_PARM_ATTRIBUTE *) 0 };

typedef struct t_parm_attribute_val_struct T_PARM_ATTRIBUTE_VAL;
struct t_parm_attribute_val_struct
{
	T_PARM_ATTRIBUTE_VAL	*next;
	char			*t_parm_attribute_val;
	FTCHAR			t_parm_attribute_val_on;
	FTCHAR			t_parm_attribute_val_off;
	FTCHAR			t_parm_attribute_val_on_ok;
	FTCHAR			t_parm_attribute_val_off_ok;
};
T_PARM_ATTRIBUTE_VAL	*sT_parm_attribute_val_ptr =
						{ (T_PARM_ATTRIBUTE_VAL *) 0 };

#define MAX_PARM_ATTRIBUTE_OUT 10
int	sT_parm_attribute_outno = { 0 } ;
char	*sT_parm_attribute_out[ MAX_PARM_ATTRIBUTE_OUT ] = {NULL};
FTCHAR	sT_parm_attribute_out_on = { 0 };
FTCHAR	sT_parm_attribute_out_off = { 0 };

char	*T_ibm_attribute = { NULL };
FTCHAR	T_ibm_attribute_ftattrs[ 3 ][ 5 ] = { 0 };

extern	char	*my_tparm();

/**************************************************************************
* fct_attribute_init
**************************************************************************/
fct_attribute_init()
{
}
/**************************************************************************
* extra_attribute
*	Examine terminal description file line for info relevant to
*	attributes and install if found.
**************************************************************************/
extra_attribute( name, string, attribute_on_string, attribute_off_string )
	char	*name;
	char	*string;
	char	*attribute_on_string;
	char	*attribute_off_string;
{
	int	match;
	char	*dec_encode();
	char	*break_dash_string();

	match = 1;
	if ( strcmp( name, "graphics_escape_control" ) == 0 )
	{
		/**********************************************************
		* Escape followed by a control character is a single
		* graphics character.
		**********************************************************/
		xF_graphics_escape_control = get_optional_value( string, 1 );
	}
	else if ( strcmp( name, "graphics_escape_delete" ) == 0 )
	{
		/**********************************************************
		* Escape followed by delete is a single graphics character.
		**********************************************************/
		xF_graphics_escape_delete = get_optional_value( string, 1 );
	}
	else if ( strcmp( name, "delete_is_a_character" ) == 0 )
	{
		/**********************************************************
		* 0x7f is a single character.
		**********************************************************/
		xF_delete_is_a_character = get_optional_value( string, 1 );
	}
	else if ( strcmp( name, "attribute" ) == 0 )
	{
		/**********************************************************
		* "string" turns "attribute_on_string"  attributes on and 
		* "attributes_off_string" attributes off.
		**********************************************************/
		ex_attribute( string,
			attribute_on_string, attribute_off_string );
	}
	else if ( strcmp( name, "attribute_next" ) == 0 )
	{
		/**********************************************************
		* "string" turns "attribute_on_string"  attributes on and 
		* "attributes_off_string" attributes off but only for the
		* next character. E.g. make the next character a line drawing
		* graphic.
		**********************************************************/
		ex_attribute_next( string,
			attribute_on_string, attribute_off_string );
	}
	else if ( strcmp( name, "parm_attribute" ) == 0 )
	{
		/**********************************************************
		* "string" is a form that has multiple parameters whose
		* effect is described in parm_attribute_val.
		* The basic string itself turns "attribute_on_string"
		* attributes on and "attributes_off_string" attributes off.
		* The basic strin itself also implies an optional third
		* parameter that will be on the end of
		* "attributes_off_string".
		**********************************************************/
		ex_parm_attribute( string,
			attribute_on_string, attribute_off_string );
	}
	else if ( strcmp( name, "parm_attribute_val" ) == 0 )
	{
		/**********************************************************
		* Parameters seen in "parm_attribute" that match "string"
		* turns "attribute_on_string" attributes on and 
		* "attributes_off_string" attributes off.
		* These can also be used in "parm_attribute_out".
		**********************************************************/
		ex_parm_attribute_val( string,
			attribute_on_string, attribute_off_string );
	}
	else if ( strcmp( name, "parm_attribute_out" ) == 0 )
	{
		/**********************************************************
		* "parm_attribute_out" is used to output the parm_attribute
		* strings.  The first one is to output 1 parm_val, the
		* second is to output two parm_vals, etc.
		* "string" has the side effect of 
		* turning "attribute_on_string" attributes on and 
		* "attributes_off_string" attributes off.
		**********************************************************/
		ex_parm_attribute_out( string,
			attribute_on_string, attribute_off_string );
	}
	else if ( strcmp( name, "exit_attribute_mode" ) == 0 )
	{
		/**********************************************************
		* "string" turns most attributes off and in particular
		* turns "attribute_off_string" attributes off.
		* "attribute_on_string" is not used.
		**********************************************************/
		ex_exit_attribute_mode( string,
			attribute_on_string, attribute_off_string );
	}
	else if ( strcmp( name,
			"row_address_exit_attribute_set_attr_line" ) == 0 )
	{
		/**********************************************************
		* "string" positions the cursor to a specified row,
		* turns "attribute_off_string" attributes off,
		* and sets the current line to the resulting attributes.
		* Yes, this is a real command.
		**********************************************************/
		ex_prime_cva( string,
			attribute_on_string, attribute_off_string );
	}
	else if ( strcmp( name, "ibm_attribute" ) == 0 )
	{
		char	*s[ 3 ];

		s[ 0 ] = attribute_on_string;
		s[ 1 ] = attribute_off_string;
		s[ 2 ] = break_dash_string( attribute_off_string );
		encode_ibm_attribute( s );
		T_ibm_attribute = dec_encode( string );
		inst_ibm_attribute( T_ibm_attribute );
	}
	else if ( strcmp( name, "ibm_attribute_out" ) == 0 )
	{
		T_ibm_attribute = dec_encode( string );
	}
	else
		match = 0;
	return( match );
}
/**********************************************************
* ex_exit_attribute_mode
*	"string" turns most attributes off and in particular
*	turns "attribute_off_string" attributes off.
*	"attribute_on_string" is not used.
**********************************************************/
/*ARGSUSED*/
ex_exit_attribute_mode( string, attribute_on_string, attribute_off_string )
	char	*string;
	char	*attribute_on_string;
	char	*attribute_off_string;
{
	char	*dec_encode();
	FTCHAR	attribute_encode();

	xT_exit_attribute_mode = dec_encode( string );
	xT_exit_attribute_mode_off = attribute_encode( attribute_off_string );
	inst_exit_attribute_mode( xT_exit_attribute_mode );
}
/**************************************************************************
* dec_exit_attribute_mode
*	DECODE has seen a sequence matching "exit_attribute_mode="
**************************************************************************/
dec_exit_attribute_mode()
{
	fct_exit_attribute_mode();
}
/**************************************************************************
* inst_exit_attribute_mode
*	Install the string "str" as an exit_attribute_mode sequence.
**************************************************************************/
inst_exit_attribute_mode( str )
	char	*str;
{
	dec_install( "exit_attribute_mode", (UNCHAR *) str, 
		dec_exit_attribute_mode, 0, 0,
		(char *) 0 );
}
/**************************************************************************
* fct_exit_attribute_mode
*	DECODE has seen a sequence matching "exit_attribute_mode="
**************************************************************************/
fct_exit_attribute_mode()
{
	m_clear_regular( oT_exit_attribute_mode_off );
	if ( Outwin->onscreen )
		term_exit_attribute_mode();
}
/**************************************************************************
* term_exit_attribute_mode
*	Send the exit_attribute_mode sequence to the terminal.
*	Remove the attributes turned off by that sequence from the 
*	assumed terminal attributes.
**************************************************************************/
term_exit_attribute_mode()
{
	if ( mT_exit_attribute_mode != NULL )
	{
		term_tputs( mT_exit_attribute_mode );
		M_attribute &= (~mT_exit_attribute_mode_off );
	}
}
/**************************************************************************
* row_address_exit_attribute_set_attr_line
* Syntax:
*	row_address_exit_attribute_set_attr_line--attribute_off_string=sequence
* Example:
*	row_address_exit_attribute_set_attr_line--URBDIPA=\E2%'!'%p1%+%c! p
* Fields:
*	attribute_off_string
*		the attributes turned off by this command
* Parameters in sequence:
*	%p1	row number
* Semantics:
*	Positions cursor to row %p1.
*	Turn off the attributes in "attribute_off_string".
*	Set attributes on the resulting line to the resulting attributes.
* Modules:
*	ex_prime_cva
*		Encodes fields from the terminal description file.
*	inst_prime_cva
*		Installs the  sequence in decoder tree.
*	dec_prime_cva
*		Called by decoder when sequence recognized.
*	fct_prime_cva
*		Called by dec_prime_cva for processing of the sequence.
*	term_prime_cva
*		Instantiates the sequence and outputs it to the terminal.
**************************************************************************/
/**************************************************************************
* ex_prime_cva
*	Encode fields from "row_address_exit_attribute_set_attr_line".
*	Save and install.
*	The character string "string" is the decode sequence.
*	"attribute_on_string" is not used.
*	"attribute_off_string"
**************************************************************************/
/*ARGSUSED*/
ex_prime_cva( string, attribute_on_string, attribute_off_string )
	char	*string;
	char	*attribute_on_string;
	char	*attribute_off_string;
{
	char	*dec_encode();
	FTCHAR	attribute_encode();

	xT_prime_cva = dec_encode( string );
	xT_prime_cva_off = attribute_encode( attribute_off_string );
	inst_prime_cva( xT_prime_cva );
}
/**************************************************************************
* dec_prime_cva
*	See "row_address_exit_attribute_set_attr_line"
**************************************************************************/
/*ARGSUSED*/
dec_prime_cva( not_used, parm_ptr, parms_valid, parm )
	int	not_used;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
{
	int	row;

	row = parm[ 0 ];
	fct_prime_cva( row );
}
/**************************************************************************
* inst_prime_cva
*	See "row_address_exit_attribute_set_attr_line"
**************************************************************************/
inst_prime_cva( str )
	char	*str;
{
	dec_install( "row_address_exit_attribute_set_attr_line",
		(UNCHAR *) str, 
		dec_prime_cva, 0, CURSOR_OPTION,
		(char *) 0 );
}
/**************************************************************************
* fct_prime_cva
*	ACTION module for 'row_address_exit_attribute_set_attr_line'.
**************************************************************************/
#include "ftterm.h"
fct_prime_cva( row )
	int	row;
{
	int	row_on_screen;

	if ( row < 0 )
		Outwin->row = 0;
					/* limit row to bottom of screen */
	else if ( row > Outwin->display_row_bottom )
		Outwin->row = Outwin->display_row_bottom;
	else
		Outwin->row = row;
	Outwin->col = 0;
	Outwin->xenl = 0;
	Outwin->real_xenl = 0;
	m_clear_regular( oT_prime_cva_off );
	d_set_attr_line( Outwin->row );
	if ( Outwin->onscreen )
	{
		if ( Outwin->fullscreen )
			term_prime_cva( Outwin->row );
		else
		{
			s_cursor_w_pan();
			/**************************************************
			* calculate row on screen for Outwin->row
			**************************************************/
			row_on_screen = s_calc_row_on_screen();
			term_prime_cva( row_on_screen );
		}
	}
}
/**************************************************************************
* term_prime_cva
*	See "row_address_exit_attribute_set_attr_line"
**************************************************************************/
term_prime_cva( row )
	int	row;
{
	char	*p;
	int	parm[ 2 ];			/* 1 used - must be >= 2 - %i */
	char	*string_parm[ 1 ];		/* not used */

	if ( mT_prime_cva != NULL )
	{
		parm[ 0 ] = row;
		p = my_tparm( mT_prime_cva, parm, string_parm, -1 );
		my_tputs( p, 1 );
		M_attribute &= (~mT_prime_cva_off );
	}
}
/**************************************************************************
* ex_attribute
*	Parse "attribute=" line.
**************************************************************************/
#include "linklast.h"
ex_attribute( string, attribute_on_string, attribute_off_string )
	char	*string;
	char	*attribute_on_string;
	char	*attribute_off_string;
{
	char	*dec_encode();
	FTCHAR	attribute_encode();
	char	*break_dash_string();
	char	*attribute_on_ok_string;
	char	*attribute_off_ok_string;
	T_ATTRIBUTE	*t_attribute_ptr;
	long	*mymalloc();

	attribute_on_ok_string =  break_dash_string( attribute_off_string );
	attribute_off_ok_string = break_dash_string( attribute_on_ok_string );
	t_attribute_ptr = 
		(T_ATTRIBUTE *) mymalloc( sizeof( T_ATTRIBUTE ), 
					  "attribute" );
	t_attribute_ptr->next = (T_ATTRIBUTE *) 0;
	link_last( (T_STRUCT *) t_attribute_ptr,
		   (T_STRUCT *) &xT_attribute_ptr );
	t_attribute_ptr->t_attribute = dec_encode( string );
	t_attribute_ptr->t_attribute_on =
				attribute_encode( attribute_on_string );
	t_attribute_ptr->t_attribute_off =
				attribute_encode( attribute_off_string );
	t_attribute_ptr->t_attribute_on_ok = 
				attribute_encode( attribute_on_ok_string );
	t_attribute_ptr->t_attribute_off_ok =
				attribute_encode( attribute_off_ok_string );
	inst_attribute( t_attribute_ptr->t_attribute, 0, t_attribute_ptr );
}
/**************************************************************************
* break_dash_string
*	Find the first '-' in the string "string".  Replace it with a
*	null and return a pointer to the string following the dash.
*	Return NULL if "string" is NULL or does not contain a dash.
*	This is used to separate optional arguments in the terminal
*	description file.
**************************************************************************/
char *
break_dash_string( string )
	char	*string;
{
	char	*s;

					/* if NULL input return NULL */
	if ( string == NULL )
		return( NULL );
	for ( s = string; *s != '\0'; s++ )
	{
					/* if - in string, replace with 0
					   to terminate string and return
					   pointer to next char */
		if ( *s == '-' )
		{
			*s = '\0';
			s++;
			return( s );
		}
	}
					/* if no - in string return NULL */
	return( NULL );
}
/**************************************************************************
* dec_attribute
*	DECODE module for 'attribute'.
**************************************************************************/
/*ARGSUSED*/
dec_attribute( unused, t_attribute_ptr )
	int		unused;
	T_ATTRIBUTE	*t_attribute_ptr;
{
	fct_attribute( t_attribute_ptr );
}
/**************************************************************************
* inst_attribute
**************************************************************************/
inst_attribute( str, parmno, t_attribute_ptr )
	char		*str;
	int		parmno;
	T_ATTRIBUTE	*t_attribute_ptr;
{
	dec_install( "attribute", (UNCHAR *) str,
		dec_attribute, parmno, 0,
		(char *) t_attribute_ptr );
}
/**************************************************************************
* fct_attribute
*	ACTION module for 'attribute'.
**************************************************************************/
fct_attribute( t_attribute_ptr )
	T_ATTRIBUTE	*t_attribute_ptr;
{
	m_set_regular(   t_attribute_ptr->t_attribute_on );
	m_clear_regular( t_attribute_ptr->t_attribute_off );
	if ( Outwin->onscreen )
		term_attribute( t_attribute_ptr );
}
/**************************************************************************
* term_attribute
*	TERMINAL OUTPUT module for 'attribute'.
**************************************************************************/
term_attribute( t_attribute_ptr )
	T_ATTRIBUTE	*t_attribute_ptr;
{
	char		*attributestr;

	attributestr = t_attribute_ptr->t_attribute;
	if ( attributestr != NULL )
	{
		term_tputs( attributestr );
		M_attribute |=       t_attribute_ptr->t_attribute_on;
		M_attribute &= ( ~ ( t_attribute_ptr->t_attribute_off ) );
	}
}
/**************************************************************************
* term_se_attribute
*	An operation has taken place that has the side effect of setting
*	the terminals assumed attributes to the attributes "ftattrs".
**************************************************************************/
term_se_attribute( ftattrs )
	FTCHAR	ftattrs;
{
	M_attribute = ftattrs;
}
/**************************************************************************
* term_se_attribute_off
*	An operation has taken place that has the side effect of turning
*	off the attributes "ftattrs" in the terminals assumed attributes.
**************************************************************************/
term_se_attribute_off( ftattrs )
	FTCHAR	ftattrs;
{
	M_attribute &= (~ftattrs);
}
/**************************************************************************
* ex_attribute_next
*	Encode and install an "attribute_next" line from a terminal
*	description.
**************************************************************************/
ex_attribute_next( string, attribute_on_string, attribute_off_string )
	char	*string;
	char	*attribute_on_string;
	char	*attribute_off_string;
{
	char	*dec_encode();
	FTCHAR	attribute_encode();
	char	*break_dash_string();
	char	*attribute_on_ok_string;
	char	*attribute_off_ok_string;
	T_ATTRIBUTE_NEXT	*t_attribute_next_ptr;
	long	*mymalloc();

	attribute_on_ok_string =  break_dash_string( attribute_off_string );
	attribute_off_ok_string = break_dash_string( attribute_on_ok_string );
	t_attribute_next_ptr = (T_ATTRIBUTE_NEXT *) 
				mymalloc( sizeof( T_ATTRIBUTE_NEXT ), 
					  "attribute_next" );
	t_attribute_next_ptr->next = (T_ATTRIBUTE_NEXT *) 0;
	link_last( (T_STRUCT *) t_attribute_next_ptr,
		   (T_STRUCT *) &xT_attribute_next_ptr );
	t_attribute_next_ptr->t_attribute_next = dec_encode( string );
	t_attribute_next_ptr->t_attribute_next_on =
				attribute_encode( attribute_on_string );
	t_attribute_next_ptr->t_attribute_next_off = 
				attribute_encode( attribute_off_string );
	t_attribute_next_ptr->t_attribute_next_on_ok =
				attribute_encode( attribute_on_ok_string );
	t_attribute_next_ptr->t_attribute_next_off_ok = 
				attribute_encode( attribute_off_ok_string );
	inst_attribute_next( t_attribute_next_ptr->t_attribute_next,
				t_attribute_next_ptr );
}
/**************************************************************************
* dec_attribute_next
*	DECODE module for 'attribute_next'.
**************************************************************************/
/*ARGSUSED*/
dec_attribute_next( unused, t_attribute_next_ptr, parms_valid, parm )
	int	unused;
	T_ATTRIBUTE_NEXT	*t_attribute_next_ptr;
	int	parms_valid;
	int	parm[];
{
	fct_attribute_next( t_attribute_next_ptr, (UNCHAR) parm[ 0 ],
		t_attribute_next_ptr->t_attribute_next_on );
}
/**************************************************************************
* inst_attribute_next
*	INSTALL module for 'attribute_next'.
**************************************************************************/
inst_attribute_next( str, t_attribute_next_ptr )
	char	*str;
	T_ATTRIBUTE_NEXT	*t_attribute_next_ptr;
{
	dec_install( "attribute_next", (UNCHAR *) str,
		dec_attribute_next, 0,
		BYTE_PARM_FOLLOWS_OPTION,
		(char *) t_attribute_next_ptr );
}
/**************************************************************************
* term_attribute_next
*	TERMINAL OUTPUT module for 'attribute_next'.
**************************************************************************/
term_attribute_next( t_attribute_next_ptr )
	T_ATTRIBUTE_NEXT	*t_attribute_next_ptr;
{
	char	*attribute_nextstr;

	attribute_nextstr = t_attribute_next_ptr->t_attribute_next;
	if ( attribute_nextstr != NULL )
		term_tputs( attribute_nextstr );
}
/**************************************************************************
* m_set_regular
*	Turn on the attributes "ftattrs" for the current output window.
**************************************************************************/
m_set_regular( ftattrs )
	FTCHAR	ftattrs;
{
	Outwin->ftattrs |= ftattrs;
}
/**************************************************************************
* m_clear_regular
*	Turn off the attributes "ftattrs" for the current output window.
**************************************************************************/
m_clear_regular( ftattrs )
	FTCHAR	ftattrs;
{
	Outwin->ftattrs &= (~ftattrs );
}
/**************************************************************************
* win_se_attribute
*	An operation has taken place that has the side effect of setting
*	the attributes of the current output window to "ftattrs".
*	This does not affect the character set selections of the window.
**************************************************************************/
win_se_attribute( ftattrs )
	FTCHAR	ftattrs;
{
	Outwin->ftattrs &= F_character_set_attributes;	/* regular off */
	Outwin->ftattrs |= ( ftattrs & (~F_character_set_attributes) );
}
/**************************************************************************
* win_se_attributes_off
*	An operation has taken place that has the side effect of turning
*	off the attributes "ftattrs" on the current output window.
**************************************************************************/
win_se_attributes_off( ftattrs )
	FTCHAR	ftattrs;
{
	Outwin->ftattrs &= (~ftattrs);
}
/**************************************************************************
* t_sync_attr
*	Syncronize the attributes of the terminal to the attributes
*	specified in "ftattrs".
*	Return 1 if an "attribute next" sequence has been output.
*	Return 0 otherwise.
**************************************************************************/
t_sync_attr( ftattrs )		/* 1=attribute next 0=not */
	FTCHAR	ftattrs;
{
	FTCHAR	change;
	FTCHAR	on;
	FTCHAR	off;
	FTCHAR	on_ok;
	FTCHAR	off_ok;
	char	buff[ 80 ];
	T_ATTRIBUTE		*t_attribute_ptr;
	T_ATTRIBUTE_NEXT	*t_attribute_next_ptr;

	/******************************************************************
	* Ignore character sets.
	******************************************************************/
	ftattrs &= (~F_character_set_attributes);
	/******************************************************************
	* Exit if terminal matches attributes.
	******************************************************************/
	change = ftattrs ^ M_attribute;
	if ( change == 0 )
		return( 0 );
	/******************************************************************
	* If "exit_attribute_mode_off" will turn off any attributes that
	* should be off, use it even if it turns off some that should be
	* on.
	* This may not be optimal but is conservative and prevents unwanted
	* side effects of some terminal's behaviors from getting us in
	* trouble.
	******************************************************************/
	off = (~ftattrs) & M_attribute;
	if ( off & mT_exit_attribute_mode_off )
	{
		term_exit_attribute_mode();
		change = ftattrs ^ M_attribute;
		if ( change == 0 )
			return( 0 );
	}
	if ( T_ibm_attribute != (char *) 0 )
	{
		t_sync_ibm_attribute( ftattrs );
		if ( ftattrs == M_attribute )
			return( 0 );
	}
	/******************************************************************
	* Try "parm_attribute=" and "parm_attribute_val" types.
	******************************************************************/
	t_sync_attr_parm( ftattrs );
	if ( ftattrs == M_attribute )
		return( 0 );
	/******************************************************************
	* Try "attribute=" types of attributes.
	******************************************************************/
	for (   t_attribute_ptr = mT_attribute_ptr;
		t_attribute_ptr != (T_ATTRIBUTE *) 0;
		t_attribute_ptr = t_attribute_ptr->next )
	{
		if ( ftattrs == M_attribute )
			return( 0 );
		on =     t_attribute_ptr->t_attribute_on;
		off =    t_attribute_ptr->t_attribute_off;
		on_ok =  t_attribute_ptr->t_attribute_on_ok;
		off_ok = t_attribute_ptr->t_attribute_off_ok;
					/* if    turn on  wanted on
					      or turn off wanted off
					   and not turn on  wanted off (or ok )
					   and not turn off wanted on  (or ok )
					*/
		if (  (  ( on  &    ftattrs   & ( ~M_attribute ) )
		      || ( off & ( ~ftattrs ) &    M_attribute   ) )
		   && (  ( on  & ( ~ftattrs ) & ( ~on_ok       ) ) == 0  )
		   && (  ( off &    ftattrs   & ( ~off_ok      ) ) == 0  ) )
		{
				term_attribute( t_attribute_ptr );
		}
	}
	if ( ftattrs == M_attribute )
		return( 0 );
	/******************************************************************
	* Try "attribute_next=" types of attributes.
	******************************************************************/
	for (   t_attribute_next_ptr = mT_attribute_next_ptr;
		t_attribute_next_ptr != (T_ATTRIBUTE_NEXT *) 0;
		t_attribute_next_ptr = t_attribute_next_ptr->next )
	{
		on =     t_attribute_next_ptr->t_attribute_next_on;
		off =    t_attribute_next_ptr->t_attribute_next_off;
		on_ok =  t_attribute_next_ptr->t_attribute_next_on_ok;
		off_ok = t_attribute_next_ptr->t_attribute_next_off_ok;
					/* if    turn on  wanted on
					      or turn off wanted off
					   and not turn on  wanted off (or ok )
					   and not turn off wanted on  (or ok )
					*/
		if (  (  ( on  &    ftattrs   & ( ~M_attribute ) )
		      || ( off & ( ~ftattrs ) &    M_attribute   ) )
		   && (  ( on  & ( ~ftattrs ) & ( ~on_ok       ) ) == 0  )
		   && (  ( off &    ftattrs   & ( ~off_ok      ) ) == 0  ) )
		{
				term_attribute_next( t_attribute_next_ptr );
				return( 1 );
		}
	}
	/******************************************************************
	* Complain that it couldn't be done.
	******************************************************************/
	if ( ftattrs != M_attribute )
	{
		sprintf( buff, "t_sync_attr-%lx-%lx", ftattrs, M_attribute );
		error_record_msg( buff );
		term_beep();
	}
	return( 0 );
}
/**************************************************************************
* t_sync_attr_parm
*	Try to syncronize the attributes of the terminal to the attributes
*	specified in "ftattrs"useing "parm_attribute=" and 
*	"parm_attribute_val" types of attributes.
**************************************************************************/
t_sync_attr_parm( ftattrs )
	FTCHAR	ftattrs;
{
	int	j;
	FTCHAR	current;
	FTCHAR	change;
	FTCHAR	on;
	FTCHAR	off;
	FTCHAR	on_ok;
	FTCHAR	off_ok;
	FTCHAR	attribute_on;
	FTCHAR	attribute_off;
	char	*string_parms[ MAX_PARM ];
	T_PARM_ATTRIBUTE_VAL	*t_parm_attribute_val_ptr;

	attribute_on = 0;
	attribute_off = 0;
	current = M_attribute;
	current |=    mT_parm_attribute_out_on;
	current &= ( ~mT_parm_attribute_out_off );
	j = 0;
	for (	t_parm_attribute_val_ptr = mT_parm_attribute_val_ptr;
		t_parm_attribute_val_ptr != (T_PARM_ATTRIBUTE_VAL *) 0;
		t_parm_attribute_val_ptr = t_parm_attribute_val_ptr->next )
	{
		change = ftattrs ^ current;
		if ( change == 0 )
			break;
		on =     t_parm_attribute_val_ptr->t_parm_attribute_val_on;
		off =    t_parm_attribute_val_ptr->t_parm_attribute_val_off;
		on_ok =  t_parm_attribute_val_ptr->t_parm_attribute_val_on_ok;
		off_ok = t_parm_attribute_val_ptr->t_parm_attribute_val_off_ok;
					/* if    turn on  wanted on
					      or turn off wanted off
					   and not turn on  wanted off (or ok )
					   and not turn off wanted on  (or ok )
					*/
		if (  (  ( on  &    ftattrs   & ( ~current     ) )
		      || ( off & ( ~ftattrs ) &    current       ) )
		   && (  ( on  & ( ~ftattrs ) & ( ~on_ok       ) ) == 0  )
		   && (  ( off &    ftattrs   & ( ~off_ok      ) ) == 0  ) )
		{
			string_parms[ j++ ] = 
				t_parm_attribute_val_ptr->t_parm_attribute_val;
			attribute_on |= on;
			attribute_on &= ( ~off );
			attribute_off |= off;
			attribute_off &= ( ~on );
		}
		current |= attribute_on;
		current &= ( ~attribute_off );
	}
	/******************************************************************
	* If we found some that will work and they make a difference,
	* output them.
	******************************************************************/
	if ( ( j > 0 ) && ( current != M_attribute ) )
	{
		term_parm_attribute( j - 1, string_parms, 
				     attribute_on, attribute_off );
	}
}
/**************************************************************************
* attribute_encode
*	Encode the attribute string "s" into the appropriate bits in a
*	FACET/TERM attribute word ( 24 bits ):
*		ponmlkjihgfedcbaURBDOIPA........
*	'.' is ignored because it is used as a spacer.  Anything else
*	provokes a complaint.
**************************************************************************/
FTCHAR
attribute_encode( s )
	char	*s;
{
	char	c;
	FTCHAR	result;
	char	buff[2];

	if ( s == NULL )
		return( 0 );
	result = 0;
	while ( ( c = *s++ ) != '\0' )
	{
		switch( c )
		{
		case 'U': result |= ATTR_UNDERLINE;	break;
		case 'R': result |= ATTR_REVERSE;	break;
		case 'B': result |= ATTR_BLINK;		break;
		case 'D': result |= ATTR_DIM;		break;
		case 'O': result |= ATTR_BOLD;		break;
		case 'I': result |= ATTR_INVIS;		break;
		case 'P': result |= ATTR_PROTECT;	break;
		case 'A': result |= ATTR_ALTCHARSET;	break;
#ifndef FTCHAR_SHORT
		case 'a': result |= ATTR_a;		break;
		case 'b': result |= ATTR_b;		break;
		case 'c': result |= ATTR_c;		break;
		case 'd': result |= ATTR_d;		break;
		case 'e': result |= ATTR_e;		break;
		case 'f': result |= ATTR_f;		break;
		case 'g': result |= ATTR_g;		break;
		case 'h': result |= ATTR_h;		break;
		case 'i': result |= ATTR_i;		break;
		case 'j': result |= ATTR_j;		break;
		case 'k': result |= ATTR_k;		break;
		case 'l': result |= ATTR_l;		break;
		case 'm': result |= ATTR_m;		break;
		case 'n': result |= ATTR_n;		break;
		case 'o': result |= ATTR_o;		break;
		case 'p': result |= ATTR_p;		break;
#endif
		case '.': /* no op - spacer */		break;
		default:
			buff[ 0 ] = c;
			buff[ 1 ] = '\0';
			printf( "Invalid attribute '%s'\n", buff );
			break;
		}
	}
	return( result );
}
/**************************************************************************
* ex_parm_attribute
*	Encode and install a "parm_attribute" terminal description.
*	An optional third argument may be attached to the attribute_off_string.
*	If present, it is an implied attribute.
*	E.g.	\E[;....m   implies   \E[0;....m  on most but not all 
*		terminals.
**************************************************************************/
ex_parm_attribute( string, attribute_on_string, attribute_off_string )
	char	*string;
	char	*attribute_on_string;
	char	*attribute_off_string;
{
	FTCHAR	attribute_encode();
	char	*break_dash_string();
	char	*implied_val;
	char	*dec_encode();
	T_PARM_ATTRIBUTE	*t_parm_attribute_ptr;
	long	*mymalloc();

	implied_val = break_dash_string( attribute_off_string );
	t_parm_attribute_ptr = (T_PARM_ATTRIBUTE *) 
				mymalloc( sizeof( T_PARM_ATTRIBUTE ), 
					  "parm_attribute" );
	t_parm_attribute_ptr->next = (T_PARM_ATTRIBUTE *) 0;
	link_last( (T_STRUCT *) t_parm_attribute_ptr,
		   (T_STRUCT *) &xT_parm_attribute_ptr );
	t_parm_attribute_ptr->t_parm_attribute_on = 
			attribute_encode( attribute_on_string );
	t_parm_attribute_ptr->t_parm_attribute_off =
			attribute_encode( attribute_off_string );
	if ( implied_val != NULL )
	{
		t_parm_attribute_ptr->t_parm_attribute_implied_val = 
					dec_encode( implied_val );
	}
	else
	{
		t_parm_attribute_ptr->t_parm_attribute_implied_val = 
					NULL;
	}
	ex_inst_parm_attribute( string, t_parm_attribute_ptr );
}
/**************************************************************************
* ex_inst_parm_attribute
*	Parse line for "parm_attribute".
**************************************************************************/
#define MAX_STORE_LEN 256
ex_inst_parm_attribute( string, parm_attribute_ptr )
	char	*string;
	T_PARM_ATTRIBUTE	*parm_attribute_ptr;
{
	char	*encoded;
	char	front[ 80 ];
	char	back[ 80 ];
	char	newstring[ 160 ];
	char	buff[ 80 ];
	char	special;
	int	repeat;
	int	i;
	int	j;
	int	k;
	char	*temp_encode();
	char	storage[ MAX_STORE_LEN + 1 ];
	char	*dec_encode();

	if ( parse_for_percent_M( string, front, back, &special, &repeat ) == 0)
	{
		encoded = dec_encode( string );
		inst_parm_attribute( encoded, parm_attribute_ptr );
	}
	else if ( special == 'M' )
	{
	    for ( i = 1; i <= repeat; i++ )
	    {
						/* add \E[ */
			strcpy( newstring, front );
						/* add %p1%d;%p2%d;%p3%d; */
			for ( k = 1; k <= i; k++ )
			{
				sprintf( buff, "%%p%d%%d", k );
				strcat( newstring, buff );
				if ( k != i )
					strcat( newstring, ";" );
			}
						/* add m */
			strcat( newstring, back );
			if ( strlen( newstring ) < MAX_STORE_LEN )
			{
				encoded = temp_encode( newstring, storage );
				inst_parm_attribute( encoded,
							parm_attribute_ptr );
			}
			else
			{
				printf( "parm_attribute string is too big %d\n",
					strlen( newstring ) );
				printf( "'%s'\n", newstring );
			}
	    }
	}
	else
	{
	    for ( i = 1; i <= repeat; i++ )
	    {
		for ( j = i; j >= 0; j-- )
		{
						/* add \E[ */
			strcpy( newstring, front );
						/* add %p1%d;%p2%d;%p3%d; */
			for ( k = 1; k <= j; k++ )
			{
				sprintf( buff, "%%p%d%%d", k );
				strcat( newstring, buff );
				if ( k != i )
					strcat( newstring, ";" );
			}
						/* add %C4?%p4%d */
			if ( k <= i )
			{
				sprintf( buff, "%cC%d%c%%p%d%%d",
						'%', k, special, k );
				strcat( newstring, buff );
				if ( k < i )
					strcat( newstring, ";" );
				k++;
			}
						/* add %m5?;%m6?;%m7? */
			for(	; k <= i; k++ )
			{
				sprintf( buff, "%%m%d%c", k, special );
				strcat( newstring, buff );
				if ( k != i )
					strcat( newstring, ";" );
			}
						/* add m */
			strcat( newstring, back );
			if ( strlen( newstring ) < MAX_STORE_LEN )
			{
				encoded = temp_encode( newstring, storage );
				inst_parm_attribute( encoded, 
							parm_attribute_ptr );
			}
			else
			{
				printf( "parm_attribute string is too big %d\n",
					strlen( newstring ) );
				printf( "'%s'\n", newstring );
			}
		}
	    }
	}
}
/**************************************************************************
* ex_parm_attribute_val
*	Encode and install a "parm_attribute_val" terminal description.
*	Two optional arguments may be attached to the attribute_off_string.
*	These indicate attributes that it is ok to turn on as a side
*	effect and attributes that it is ok to turn off as a side effect.
*	That is, even if they are turned on (turned off) and should not
*	be, then they can be turned off (turned on) in subsequent strings.
**************************************************************************/
ex_parm_attribute_val( string, attribute_on_string, attribute_off_string )
	char	*string;
	char	*attribute_on_string;
	char	*attribute_off_string;
{
	FTCHAR	attribute_on;
	FTCHAR	attribute_off;
	int	complain;
	FTCHAR	attribute_encode();
	char	*break_dash_string();
	char	*attribute_on_ok_string;
	char	*attribute_off_ok_string;
	T_PARM_ATTRIBUTE_VAL	*t_parm_attribute_val_ptr;
	long	*mymalloc();
	char	*dec_encode();

	attribute_on_ok_string =  break_dash_string( attribute_off_string );
	attribute_off_ok_string = break_dash_string( attribute_on_ok_string );
	t_parm_attribute_val_ptr = (T_PARM_ATTRIBUTE_VAL *) 
				mymalloc( sizeof( T_PARM_ATTRIBUTE_VAL ), 
				"parm_attribute_val" );
	t_parm_attribute_val_ptr->next = (T_PARM_ATTRIBUTE_VAL *) 0;
	link_last( (T_STRUCT *) t_parm_attribute_val_ptr,
		   (T_STRUCT *) &xT_parm_attribute_val_ptr );
	if (  ( attribute_on_string != NULL )
	   && ( *attribute_on_string == '*' ) )
	{
		t_parm_attribute_val_ptr->t_parm_attribute_val_on = 
			attribute_encode( &attribute_on_string[ 1 ] );
		complain = 0;
	}
	else
	{
		t_parm_attribute_val_ptr->t_parm_attribute_val_on = 
			attribute_on =
			attribute_encode( attribute_on_string );
		complain = 1;
	}
	t_parm_attribute_val_ptr->t_parm_attribute_val_off =
			attribute_off =
			attribute_encode( attribute_off_string );
	t_parm_attribute_val_ptr->t_parm_attribute_val_on_ok =
			attribute_encode( attribute_on_ok_string );
	t_parm_attribute_val_ptr->t_parm_attribute_val_off_ok =
			attribute_encode( attribute_off_ok_string );
	if (  complain 
	   && ( attribute_on == 0 ) && ( attribute_off == 0 ) )
	{
		printf( "parm_attribute_val has no effect\n" );
	}
	t_parm_attribute_val_ptr->t_parm_attribute_val = dec_encode( string );
}
/**************************************************************************
* ex_parm_attribute_out
*	Parse line for "parm_attribute_out".
**************************************************************************/
ex_parm_attribute_out( string, attribute_on_string, attribute_off_string )
	char	*string;
	char	*attribute_on_string;
	char	*attribute_off_string;
{
	char	*dec_encode();
	FTCHAR	attribute_encode();

	if ( xT_parm_attribute_outno < MAX_PARM_ATTRIBUTE_OUT )
	{
		xT_parm_attribute_out_on = 
				attribute_encode( attribute_on_string );
		xT_parm_attribute_out_off =
				attribute_encode( attribute_off_string );
		xT_parm_attribute_out[ xT_parm_attribute_outno ] = 
				dec_encode( string );
		xT_parm_attribute_outno++;
	}
	else
	{
		printf( "Too many parm_attribute_out\n" );
	}
}
/**************************************************************************
* dec_parm_attribute
*	DECODE module for 'parm_attribute'.
**************************************************************************/
/*ARGSUSED*/
dec_parm_attribute( unused, t_parm_attribute_ptr, parms_valid, parm,
					string_parm, string_parms_valid )
	int	unused;
	T_PARM_ATTRIBUTE	*t_parm_attribute_ptr;
	int	parms_valid;
	int	parm[];
	char	*string_parm[];
	int	string_parms_valid;
{
	fct_parm_attribute( t_parm_attribute_ptr, parms_valid, parm,
					string_parm, string_parms_valid );
}
/**************************************************************************
* inst_parm_attribute
*	INSTALL module for 'parm_attribute'.
**************************************************************************/
inst_parm_attribute( str, t_parm_attribute_ptr )
	char	*str;
	T_PARM_ATTRIBUTE	*t_parm_attribute_ptr;
{
	dec_install( "parm_attribute", (UNCHAR *) str,
		dec_parm_attribute, 0, CURSOR_OPTION,
		(char *) t_parm_attribute_ptr );
}
/**************************************************************************
* fct_parm_attribute
*	ACTION module for 'parm_attribute'.
**************************************************************************/
#define MAX_STRING_BUFF 256
fct_parm_attribute( t_parm_attribute_ptr, parms_valid, parm, 
					string_parm, string_parms_valid )
	T_PARM_ATTRIBUTE	*t_parm_attribute_ptr;
	int	parms_valid;
	int	parm[];
	char	*string_parm[];
	int	string_parms_valid;
{
	FTCHAR	on;
	FTCHAR	off;
	FTCHAR	attribute_on;
	FTCHAR	attribute_off;
	int	i;
	int	out_parm_number;
	int	mask;
	char	buff[ 100 ];
	char	*string_parms[ MAX_PARM ];
	char	string_buff[ MAX_STRING_BUFF ];
	char	*s;
	char	*f;
	char	*implied_val;	/* parm_attribute val implied by 
				   t_parm_attribute_ptr string */
	T_PARM_ATTRIBUTE_VAL	*t_parm_attribute_val_ptr;

	/******************************************************************
	* Fixed effects of this parm_attribute string.
	******************************************************************/
	attribute_on  = t_parm_attribute_ptr->t_parm_attribute_on;
	m_set_regular( attribute_on );
	attribute_off = t_parm_attribute_ptr->t_parm_attribute_off;
	m_clear_regular( attribute_off );
	/******************************************************************
	* Some parm_attribute strings have implied parameters, such as
	* a leading ; implies a 0.  If implied, make it real.
	******************************************************************/
	implied_val = t_parm_attribute_ptr->t_parm_attribute_implied_val;
	s = string_buff;
	out_parm_number = 0;
	if ( implied_val != NULL )
	{
		string_parms[ out_parm_number ] = s;
		while( ( *s++ = *implied_val++ ) != '\0' )
				;
		out_parm_number++;
	}
	/******************************************************************
	* For each parameter, convert it to a string if necessary and
	* point a slot in the character pointer array to it for use in
	* outputting them to the terminal.
	******************************************************************/
	for ( i = 0,                     mask = 1;
	      i < MAX_PARM;
	      i++,   out_parm_number++,  mask <<= 1 )
	{
		if ( string_parms_valid & mask )
		{
			string_parms[ out_parm_number ] = s;
			f = string_parm[ i ];
			while( ( *s++ = *f++ ) != '\0' )
				;
			s--;
			if ( parms_valid & mask )
			{
				sprintf( buff, "%d", parm[ i ] );
				f = buff;
				while( ( *s++ = *f++ ) != '\0' )
					;
			}
		}
		else if ( parms_valid & mask )
		{
			string_parms[ out_parm_number ] = s;
			sprintf( buff, "%d", parm[ i ] );
			f = buff;
			while( ( *s++ = *f++ ) != '\0' )
				;
		}
		else
			break;
		/**********************************************************
		* Try to find a "parm_attribute_val" that matches the
		* parameter.
		**********************************************************/
		for (	t_parm_attribute_val_ptr = oT_parm_attribute_val_ptr;
			t_parm_attribute_val_ptr != (T_PARM_ATTRIBUTE_VAL *) 0;
			t_parm_attribute_val_ptr = 
				t_parm_attribute_val_ptr->next )
		{
			if ( strcmp( string_parms[ out_parm_number ], 
			     t_parm_attribute_val_ptr->t_parm_attribute_val ) 
			     == 0 )
			{
				break;
			}
		}
		/**********************************************************
		* If not found, complain that the attributes of that value 
		* are not known.
		**********************************************************/
		if ( t_parm_attribute_val_ptr == (T_PARM_ATTRIBUTE_VAL *) 0 )
		{
			if ( Opt_error_ignore )
			{
			}
			else if ( Opt_error_pass )
			{
			}
			else
			{
				sprintf( buff, "parm_attr_val='%s'", 
					 string_parms[ out_parm_number ] );
				error_record_msg( buff );
				term_beep();
			}
			continue;
		}
		/**********************************************************
		* Turn on and off the appropriate attributes on the window.
		* Accumulate the total effect.
		**********************************************************/
		on = t_parm_attribute_val_ptr->t_parm_attribute_val_on;
		m_set_regular( on );
		off = t_parm_attribute_val_ptr->t_parm_attribute_val_off;
		m_clear_regular( off );
		attribute_on |= on;
		attribute_on &= (~off);
		attribute_off &= (~on);
		attribute_off |= off;
	}
	/******************************************************************
	* Complain if more parameters that "parm_attribute_out" in terminal
	* description. I.E. don't know how to output it.
	******************************************************************/
	if (  ( out_parm_number <= 0 ) 
	   || ( out_parm_number >= oT_parm_attribute_outno ) )
	{
		sprintf( buff, "parm_attribute_#_parms=%d", out_parm_number );
		error_record_msg( buff );
		term_beep();
		return;
	}
	/******************************************************************
	* Reconstruct and output to terminal
	******************************************************************/
	if ( Outwin->onscreen )
		term_parm_attribute( out_parm_number - 1, string_parms,
				     attribute_on, attribute_off );
}
/**************************************************************************
* term_parm_attribute
*	TERMINAL OUTPUT module for 'parm_attribute'.
**************************************************************************/
term_parm_attribute( parm_attribute_outno, string_parms, 
		     attribute_on, attribute_off )
	int	parm_attribute_outno;
	char	*string_parms[];
	FTCHAR	attribute_on;
	FTCHAR	attribute_off;
{
	char	*parm_attribute_outstr;
	char	*p;
	int	parms[ MAX_PARM ];

	if ( parm_attribute_outno < mT_parm_attribute_outno )
	{
		parm_attribute_outstr = 
				mT_parm_attribute_out[ parm_attribute_outno ];
		if ( parm_attribute_outstr != NULL )
		{
			p = my_tparm( parm_attribute_outstr, 
				      parms, string_parms, -1 );
			my_tputs( p, 1 );
			M_attribute |= attribute_on;
			M_attribute &= (~attribute_off );
		}
	}
}
/**************************************************************************
* dec_ibm_attribute
*	DECODE module for 'ibm_attribute'.
**************************************************************************/
/*ARGSUSED*/
dec_ibm_attribute( unused, parm_ptr, parms_valid, unused_parm,
				string_parm, string_parms_valid )
	int	unused;
	char    *parm_ptr;
	int     parms_valid;
	int     unused_parm[];
	char    *string_parm[];
	int     string_parms_valid;
{
	char	*s;
	int	valid;

	s = string_parm[ 0 ];
	valid = 1;
	if ( s[ 1 ] > 0 )
	{
		valid = 2;
		if ( s[ 2 ] > 0 )
		{
			valid = 3;
		}
	}
	fct_ibm_attribute( string_parm[ 0 ], valid );
}
/**************************************************************************
* inst_ibm_attribute
**************************************************************************/
inst_ibm_attribute( str )
	char		*str;
{
	dec_install( "ibm_attribute", (UNCHAR *) str,
		dec_ibm_attribute, 0, CURSOR_OPTION,
		(char *) 0 );
}
/**************************************************************************
* fct_ibm_attribute
*	ACTION module for 'ibm_attribute'.
**************************************************************************/
fct_ibm_attribute( string, valid )
	char	*string;
	int	valid;
{
	int	i;
	int	j;
	FTCHAR	on;
	FTCHAR	off;
	int	bits;

	on = 0;
	off = 0;
	for ( i = 0; i < valid; i++ )
	{
		bits = string[ i ] & 0x1F;
		for ( j = 0; j < 5; j++ )
		{
			if ( bits & 1 )
				on  |= T_ibm_attribute_ftattrs[ i ][ j ];
			else
				off |= T_ibm_attribute_ftattrs[ i ][ j ];
			bits >>= 1;
		}
	}
	m_set_regular(   on );
	m_clear_regular( off );
	if ( Outwin->onscreen )
		term_ibm_attribute( string, on, off );
}
/**************************************************************************
* term_ibm_attribute
*	TERMINAL OUTPUT module for 'ibm_attribute'.
**************************************************************************/
term_ibm_attribute( string, on, off )
	char	*string;
	FTCHAR	on;
	FTCHAR	off;
{
	char	*ibm_attributestr;
	char	*p;
	int	parm[ 2 ];			/* 1 used - must be >= 2 - %i */
	char	*string_parm[ 1 ];		/* not used */

	ibm_attributestr = T_ibm_attribute;
	if ( ibm_attributestr != NULL )
	{
		string_parm[ 0 ] = string;
		p = my_tparm( ibm_attributestr, parm, string_parm, -1 );
		my_tputs( p, 1 );
		M_attribute |=       on;
		M_attribute &= ( ~ ( off ) );
	}
}
encode_ibm_attribute( s )
	char	*s[];
{
	int	i;
	int	j;
	int	last;
	char	*p;
	FTCHAR	fattrs;
	FTCHAR	attribute_encode();
	char	temp[ 2 ];

	for ( i = 0; i < 3; i++ )
	{
		p = s[ i ];
		last = strlen( p );
		for ( j = 0; j < 5; j++ )
		{
			last--;
			if ( last >= 0 )
			{
				temp[ 0 ] = p[ last ];
				temp[ 1 ] = '\0';
				fattrs = attribute_encode( temp );
			}
			else
				fattrs = 0;
			T_ibm_attribute_ftattrs[ i ][ j ] = fattrs;
		}
	}
}
t_sync_ibm_attribute( ftattrs )
	FTCHAR	ftattrs;
{
	FTCHAR	on;
	FTCHAR	off;
	int	bit;
	FTCHAR	attr;
	int	i;
	int	j;
	char	string_parm[ 4 ];

	string_parm[ 3 ] = '\0'; 
	on = 0;
	off = 0;
	for ( i = 0; i < 3; i++ )
	{
		string_parm[ i ] = 0x20;
		bit = 1;
		for ( j = 0; j < 5; j++ )
		{
			attr = T_ibm_attribute_ftattrs[ i ][ j ];
			if ( attr != 0 )
			{
				if ( attr & ftattrs )
				{
					string_parm[ i ] |= bit;
					on |= attr;
				}
				else
					off |= attr;
			}
			bit <<= 1;
		}
	}
	term_ibm_attribute( string_parm, on, off );
}
