/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: hpattr.c,v 70.1 92/03/09 15:44:43 ssa Exp $ */
/**************************************************************************
* hpattr.c
*	Handle attributes for HP terminals.
**************************************************************************/
#include "hpattr_p.h"
#include "ftchar.h"
#include "ftproc.h"
#include "ftwindow.h"
#include "stdio.h"

#include "features.h"
int	sF_hp_attribute = { 0 };

#include "decode.h"
#include "hpattr.h"

char	*sT_hp_attribute = { NULL };
char	sT_hp_attribute_default;
#define HP_ATTRIBUTE_DEFAULT  '@'
char	*sT_hp_color = { NULL };
/**************************************************************************
* dec_hp_attribute
*	DECODE module for 'hp_attribute'.
**************************************************************************/
/*ARGSUSED*/
dec_hp_attribute( parmno, parm_ptr, 
		  parms_valid, parm, string_parm, string_parms_valid )
	int	parmno;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
	UNCHAR	*string_parm[];
	int	string_parms_valid;
{
	int	hp_attribute;

	hp_attribute = parm[ 0 ];
	fct_hp_attribute( hp_attribute );
}
/**************************************************************************
* inst_hp_attribute
*	INSTALL module for 'hp_attribute'.
**************************************************************************/
inst_hp_attribute( str )
	char	*str;
{
	dec_install( "hp_attribute", (UNCHAR *) str, 
		     dec_hp_attribute, 0, CURSOR_OPTION,
		     (char *) 0 );
}
/**************************************************************************
* fct_hp_attribute
*	ACTION module for 'hp_attribute'.
**************************************************************************/
fct_hp_attribute( hp_attribute )
	int	hp_attribute;
{
	int	hp_charset;

	if ( Outwin->onscreen )
		term_hp_attribute( hp_attribute );
	hp_charset = find_hp_charset( 
				Outwin->ftchars[ Outwin->row ], Outwin->col );
	set_hp_attribute( hp_attribute, hp_charset );
}
/**************************************************************************
* term_hp_attribute
*	TERMINAL OUTPUT module for 'hp_attribute'.
**************************************************************************/
term_hp_attribute( hp_attribute )
	int	hp_attribute;
{
	char	*p;
	int	parm[ 2 ];			/* 1 used - must be >= 2 - %i */
	char	*string_parm[ 1 ];
	char	*my_tparm();

	if ( mT_hp_attribute != NULL )
	{
		parm[ 0 ] = hp_attribute;
		string_parm[ 0 ] = "";
		p = my_tparm( mT_hp_attribute, parm, string_parm, -1 );
		term_tputs( p );
	}
}

char	*sT_hp_charset_norm = { (char *) 0 };
char	*sT_hp_charset_alt = { (char *) 0 };

#define MAX_HP_CHARSET_SELECT 10
char	*sT_hp_charset_select[ MAX_HP_CHARSET_SELECT ]={NULL};
int	sT_hp_charset_selectno = { 0 };

/**************************************************************************
* dec_hp_charset_alt
*	DECODE module for 'hp_charset_alt'.
*	This specifies an alternate character set number "hp_charset_alt".
**************************************************************************/
/*ARGSUSED*/
dec_hp_charset_alt( hp_charset_alt, parm_ptr )
	int	hp_charset_alt;
	char	*parm_ptr;			/* not used */
{
	fct_hp_charset_alt( hp_charset_alt );
}
/**************************************************************************
* inst_hp_charset_alt
*	INSTALL module for 'hp_charset_alt'.
**************************************************************************/
inst_hp_charset_alt( str, hp_charset_alt )
	char	*str;
	int	hp_charset_alt;
{
	dec_install( "hp_charset_alt", (UNCHAR *) str, 
		     dec_hp_charset_alt, hp_charset_alt, 0,
		     (char *) 0 );
}
/**************************************************************************
* fct_hp_charset_alt
*	ACTION module for 'hp_charset_alt'.
*	Switch to alternate character set number "hp_charset_alt".
*	If this is placed on a character that has an attribute
*	propogating through it,
*	it locks that attribute at this character position.
**************************************************************************/
fct_hp_charset_alt( hp_charset_alt )
	int	hp_charset_alt;
{
	int	hp_attribute;
	int	hp_charset;

	if ( Outwin->onscreen )
		term_hp_charset_alt( hp_charset_alt );
	hp_attribute = find_hp_attribute( 
				Outwin->ftchars[ Outwin->row ], Outwin->col );
	hp_charset = ATTR_HP_CHARSET_SET;
	if ( hp_charset_alt )
	{
		hp_charset |= ATTR_HP_CHARSET_ALT;
		hp_charset |= Outwin->hp_charset_select;
	}
	set_hp_attribute( hp_attribute, hp_charset );
}
/**************************************************************************
* term_hp_charset_alt
*	TERMINAL OUTPUT module for 'hp_charset_alt'.
*	"hp_charset_alt" specifies the alternate characeter set number.
**************************************************************************/
term_hp_charset_alt( hp_charset_alt )
	int	hp_charset_alt;
{
	char	*hp_charset_altstr;

	if ( hp_charset_alt )
		hp_charset_altstr = mT_hp_charset_alt;
	else
		hp_charset_altstr = mT_hp_charset_norm;
	if ( hp_charset_altstr != NULL )
		term_tputs( hp_charset_altstr );
}
/**************************************************************************
* dec_hp_charset_select
*	DECODE module for 'hp_charset_select'.
*	This specifies the character set that is selected when 
*	hp alternate character set ( above ) is selected.
*	"hp_charset_selectno" is the character set number.
**************************************************************************/
/*ARGSUSED*/
dec_hp_charset_select( hp_charset_selectno, parm_ptr )
	int	hp_charset_selectno;
	char	*parm_ptr;
{
	fct_hp_charset_select( hp_charset_selectno );
}
/**************************************************************************
* inst_hp_charset_select
*	INSTALL module for 'hp_charset_select'.
*	"hp_charset_selectno" is the character set number.
**************************************************************************/
inst_hp_charset_select( str, hp_charset_selectno )
	char	*str;
	int	hp_charset_selectno;
{
	dec_install( "hp_charset_select", (UNCHAR *) str, 
		     dec_hp_charset_select, hp_charset_selectno, 0,
		     (char *) 0 );
}
/**************************************************************************
* fct_hp_charset_select
*	ACTION module for 'hp_charset_select'.
*	"hp_charset_selectno" is the character set number.
**************************************************************************/
fct_hp_charset_select( hp_charset_selectno )
	int	hp_charset_selectno;
{
	if ( Outwin->onscreen )
		term_hp_charset_select( hp_charset_selectno );
	Outwin->hp_charset_select = hp_charset_selectno;
}
/**************************************************************************
* term_hp_charset_select
*	TERMINAL OUTPUT module for 'hp_charset_select'.
*	"hp_charset_selectno" is the character set number.
**************************************************************************/
int M_hp_charset_select = 0;
term_hp_charset_select( hp_charset_selectno )
	int	hp_charset_selectno;
{
	char	*hp_charset_selectstr;

	if ( hp_charset_selectno < mT_hp_charset_selectno )
	{
		hp_charset_selectstr = 
			mT_hp_charset_select[ hp_charset_selectno ];
		if ( hp_charset_selectstr != NULL )
			term_tputs( hp_charset_selectstr );
		M_hp_charset_select = hp_charset_selectno;
	}
}
/**************************************************************************
* init_hp_charset
*	Initialize hp character set select for window "win" at startup.
**************************************************************************/
init_hp_charset( win )
	FT_WIN	*win;
{
	win->hp_charset_select = 0;
}
/**************************************************************************
* modes_init_hp_charset_select
*	Initialize hp character set select for window "win" at idle.
**************************************************************************/
modes_init_hp_charset_select( win )
	FT_WIN	*win;
{
	win->hp_charset_select = 0;
}
/**************************************************************************
* win_se_hp_charset_select_normal
*	An event has occurred that had the side effect of setting the
*	hp character set selection of the window to the default.
**************************************************************************/
win_se_hp_charset_select_normal()
{
	Outwin->hp_charset_select = 0;
}
/**************************************************************************
* term_se_hp_charset_select_normal
*	An event has occurred that had the side effect of setting the
*	hp character set selection of the terminal to the default.
**************************************************************************/
term_se_hp_charset_select_normal()
{
	M_hp_charset_select = 0;
}
/**************************************************************************
* t_sync_hp_charset_select_base
*	Syncronize the terminal, if necessary, to the default 
*	hp character set selection.
**************************************************************************/
t_sync_hp_charset_select_base()
{
	if ( M_hp_charset_select )
		term_hp_charset_select( 0 );
}
/**************************************************************************
* t_sync_hp_charset_select_outwin
*	Syncronize the terminal, if necessary, to the
*	hp character set selection appropriate for "Outwin".
**************************************************************************/
t_sync_hp_charset_select_outwin()
{
	if ( Outwin->hp_charset_select != M_hp_charset_select )
		term_hp_charset_select( Outwin->hp_charset_select );
}
/**************************************************************************
* t_sync_hp_charset_selectno
*	Syncronize the terminal, if necessary, to the
*	hp character set selection "hp_charset_selectno".
**************************************************************************/
t_sync_hp_charset_selectno( hp_charset_selectno )
	int	hp_charset_selectno;
{
	if ( hp_charset_selectno != M_hp_charset_select )
		term_hp_charset_select( hp_charset_selectno );
}
/**************************************************************************
* find_hp_charset
*	Return the hp character set propogating into column "col" on the
*	window buffer row pointed to by "ftchars_row".
*	It is necessary to determine this because storing an attribute
*	at "col" locks the character set currently propogating through
*	the position.
**************************************************************************/
find_hp_charset( ftchars_row, col )
	FTCHAR	*ftchars_row;
	int	col;
{
	int	hp_charset;
	FTCHAR	*p;

	for ( p = &ftchars_row[ col ];
	      p >= ftchars_row;
	      p-- )
	{
		if ( *p & ATTR_HP_CHARSET )
		{
			hp_charset = 
			    ( *p & ATTR_HP_CHARSET ) >> ATTR_HP_CHARSET_SHIFT;
			return( hp_charset );
		}
	}
	return( 0 );
}
/**************************************************************************
* find_hp_attribute
*	Return the hp attribute propogating into column "col"
*	on the window buffer row pointed to by "ftchars_row".
*	It is necessary to determine this because
*	storing a character set at "col" 
*	locks the attribute currently propogating through the position.
**************************************************************************/
find_hp_attribute( ftchars_row, col )
	FTCHAR	*ftchars_row;
	int	col;
{
	int	hp_attribute;
	FTCHAR	*p;

	for ( p = &ftchars_row[ col ];
	      p >= ftchars_row;
	      p-- )
	{
		if ( hp_attribute = ( *p & ATTR_HP_ATTRIBUTE ) )
		{
			hp_attribute >>= ATTR_HP_ATTRIBUTE_SHIFT;
			return( hp_attribute );
		}
	}
	return( oT_hp_attribute_default );
}
/**************************************************************************
* set_hp_attribute
*	Store the attribute "hp_attribute" and the character set "hp_charset"
*	at the current cursor position in the current output window.
**************************************************************************/
set_hp_attribute( hp_attribute, hp_charset )
	int	hp_attribute;
	int	hp_charset;
{
	REG	int	col;
	REG	int	row;
	REG	FTCHAR	*p;
	FTCHAR	attribute;
	FTCHAR	charset;

	attribute = ( (FTCHAR) hp_attribute ) << ATTR_HP_ATTRIBUTE_SHIFT;
	charset =   ( (FTCHAR) hp_charset ) | ATTR_HP_CHARSET_SET;
	charset <<= ATTR_HP_CHARSET_SHIFT;
	row = Outwin->row;
	col = Outwin->col;
	p = &Outwin->ftchars[ row ][ col ];
	*p &= ( ~(ATTR_HP_ATTRIBUTE | ATTR_HP_CHARSET) );
	*p |=    ( attribute | charset );
	Outwin->row_changed[ row ] = 1;
	Outwin->col_changed[ col ] = 1;
}
/**************************************************************************
* t_hp_attribute
*	Output the hp attribute, hp character set selection, 
*	hp character set, and hp color
*	specified by the FacetTerm attribute word "ftattrs".
**************************************************************************/
t_hp_attribute( ftattrs )
	FTCHAR	ftattrs;
{
	int	hp_attribute;
	int	hp_charset;
	int	hp_charset_selectno;
	int	hp_color;

	if ( ( ftattrs & ATTR_HP_ALL ) == 0 )
		return;
	if ( ftattrs & ATTR_HP_ATTRIBUTE )
	{
		hp_attribute = 
		    (ftattrs & ATTR_HP_ATTRIBUTE) >> ATTR_HP_ATTRIBUTE_SHIFT;
		term_hp_attribute( hp_attribute );
	}
	if ( ftattrs & ATTR_HP_CHARSET )
	{
		hp_charset = 
			( ftattrs & ATTR_HP_CHARSET ) >> ATTR_HP_CHARSET_SHIFT;
		hp_charset_selectno = hp_charset & ATTR_HP_CHARSET_SELECT;
		t_sync_hp_charset_selectno( hp_charset_selectno );
		if ( hp_charset & ATTR_HP_CHARSET_ALT )
			term_hp_charset_alt( 1 );
		else
			term_hp_charset_alt( 0 );
	}
	if ( ftattrs & ATTR_HP_COLOR )
	{
		hp_color = (ftattrs & ATTR_HP_COLOR) >> ATTR_HP_COLOR_SHIFT;
		term_hp_color( hp_color );
	}
}
/******************************************************************
* COLOR
******************************************************************/
/**************************************************************************
* dec_hp_color
*	DECODE module for 'hp_color'.
*	The color number is in "parm" if "parms_valid" indicates that 
*	it is present.
**************************************************************************/
/*ARGSUSED*/
dec_hp_color( parmno, parm_ptr, 
		  parms_valid, parm, string_parm, string_parms_valid )
	int	parmno;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
	UNCHAR	*string_parm[];
	int	string_parms_valid;
{
	int	hp_color;

	if ( parms_valid & 1 )
		hp_color = parm[ 0 ];
	else
		hp_color = 0;
	hp_color &= ATTR_HP_COLOR_VALUE;
	hp_color |= ATTR_HP_COLOR_PRESENT;
	fct_hp_color( hp_color );
}
/**************************************************************************
* inst_hp_color
*	INSTALL module for 'hp_color'.
**************************************************************************/
inst_hp_color( str )
	char	*str;
{
	dec_install( "hp_color", (UNCHAR *) str, 
		     dec_hp_color, 0, CURSOR_OPTION,
		     (char *) 0 );
}
/**************************************************************************
* fct_hp_color
*	ACTION module for 'hp_color'.
*	"hp_color" is the color number.
**************************************************************************/
fct_hp_color( hp_color )
	int	hp_color;
{
	REG	int	col;
	REG	int	row;
	REG	FTCHAR	*p;
	FTCHAR	ftattrs;

	if ( Outwin->onscreen )
		term_hp_color( hp_color );

	ftattrs = ( (FTCHAR) hp_color ) << ATTR_HP_COLOR_SHIFT;
	row = Outwin->row;
	col = Outwin->col;
	p = &Outwin->ftchars[ row ][ col ];
	*p &= ( ~ATTR_HP_COLOR );
	*p |= ftattrs;
	Outwin->row_changed[ row ] = 1;
	Outwin->col_changed[ col ] = 1;
}
/**************************************************************************
* term_hp_color
*	TERMINAL OUTPUT module for 'hp_color'.
*	"hp_color" is the color number.
**************************************************************************/
term_hp_color( hp_color )
	int	hp_color;
{
	char	*p;
	int	parm[ 2 ];			/* 1 used - must be >= 2 - %i */
	char	*string_parm[ 1 ];
	char	*my_tparm();

	if ( mT_hp_color != NULL )
	{
		parm[ 0 ] = hp_color & ATTR_HP_COLOR_VALUE;
		string_parm[ 0 ] = "";
		p = my_tparm( mT_hp_color, parm, string_parm, -1 );
		term_tputs( p );
	}
}
/**************************************************************************
* extra_hp_attribute
*	TERMINAL DESCRIPTION PARSER module for:
*		hp_color
*		hp_charset_select
*		hp_charset_alt
*		hp_charset_norm
*		hp_attribute
**************************************************************************/
/*ARGSUSED*/
extra_hp_attribute( name, string, attribute_on_string, attribute_off_string )
	char	*name;
	char	*string;
	char	*attribute_on_string;
	char	*attribute_off_string;		/* not used */
{
	int	match;
	char	*dec_encode();

	match = 1;
	if ( strcmp( name, "hp_attribute" ) == 0 )
	{
		xT_hp_attribute = dec_encode( string );
		inst_hp_attribute( xT_hp_attribute );
		if ( attribute_on_string != (char *) 0 )
		{
			if ( *attribute_on_string )
				xT_hp_attribute_default = *attribute_on_string;
			else
				xT_hp_attribute_default = HP_ATTRIBUTE_DEFAULT;
		}
		else
				xT_hp_attribute_default = HP_ATTRIBUTE_DEFAULT;
		xF_hp_attribute = 1;
	}
	else if ( strcmp( name, "hp_charset_norm" ) == 0 )
	{
		xT_hp_charset_norm = dec_encode( string );
		inst_hp_charset_alt( xT_hp_charset_norm, 0 );
	}
	else if ( strcmp( name, "hp_charset_alt" ) == 0 )
	{
		xT_hp_charset_alt = dec_encode( string );
		inst_hp_charset_alt( xT_hp_charset_alt, 1 );
	}
	else if ( strcmp( name, "hp_charset_select" ) == 0 )
	{
		if ( xT_hp_charset_selectno < MAX_HP_CHARSET_SELECT )
		{
			xT_hp_charset_select[ xT_hp_charset_selectno ] = 
							dec_encode( string );
			inst_hp_charset_select( 
				xT_hp_charset_select[ xT_hp_charset_selectno ], 
				xT_hp_charset_selectno );
			xT_hp_charset_selectno++;
		}
		else
		{
			printf( "Too many hp_charset_select\n" );
		}
	}
	else if ( strcmp( name, "hp_color" ) == 0 )
	{
		xT_hp_color = dec_encode( string );
		inst_hp_color( xT_hp_color );
	}
	else
		match = 0;
	return( match );
}
