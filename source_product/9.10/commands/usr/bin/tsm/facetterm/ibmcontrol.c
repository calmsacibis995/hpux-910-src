/*****************************************************************************
** Copyright (c) 1986 - 1991 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: ibmcontrol.c,v 70.1 92/03/09 16:40:31 ssa Exp $ */
#include <stdio.h>
#include "wins.h"
#include "ftchar.h"
#include "ftproc.h"
#include "ftwindow.h"
#include "features.h"
#include "decode.h"
#include "modes.h"

#include "ftterm.h"
#define MAX_IBM_CONTROL 5
#define LEN_IBM_CONTROL 8
int	T_ibm_control_no = 0;
char	*T_ibm_control[ MAX_IBM_CONTROL ] = { (char *) 0 };
char	*T_ibm_control_default[ MAX_IBM_CONTROL ] = { (char *) 0 };
char	*T_ibm_control_force_mask[ MAX_IBM_CONTROL ] = { (char *) 0 };
char	*T_ibm_control_force_value[ MAX_IBM_CONTROL ] = { (char *) 0 };
int	T_ibm_control_kind[ MAX_IBM_CONTROL ] = { 0 };
int	T_ibm_control_auto_wrap_number = -1;
int	T_ibm_control_auto_wrap_pos = 0;
int	T_ibm_control_auto_wrap_sense = 1;
char	T_ibm_control_auto_wrap_mask = '\0';
char	*T_ibm_control_auto_wrap_on_out = (char *) 0;
char	*T_ibm_control_auto_wrap_off_out = (char *) 0;
int	T_ibm_control_auto_scroll_number = -1;
int	T_ibm_control_auto_scroll_pos = 0;
int	T_ibm_control_auto_scroll_sense = 1;
char	T_ibm_control_auto_scroll_mask = '\0';
char	L_ibm_control[ TOPWIN ][ MAX_IBM_CONTROL ][ LEN_IBM_CONTROL + 1 ] =
									{ 0 };
char	M_ibm_control[ MAX_IBM_CONTROL ][ LEN_IBM_CONTROL + 1 ] = { 0 };

/**************************************************************************
* dec_ibm_control
*	DECODE module for 'ibm_control'.
**************************************************************************/
/*ARGSUSED*/
dec_ibm_control( ibm_control_no, parm_ptr, parms_valid, parm,
				string_parm, string_parms_valid )
	int	ibm_control_no;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
	char    *string_parm[];
	int     string_parms_valid;
{
	fct_ibm_control( ibm_control_no, string_parm[ 0 ] );
}
/**************************************************************************
* inst_ibm_control
*	INSTALL module for 'ibm_control'.
**************************************************************************/
inst_ibm_control( str, ibm_control_no )
	char	*str;
	int	ibm_control_no;
{
	dec_install( "ibm_control", (UNCHAR *) str, 
			dec_ibm_control, ibm_control_no, CURSOR_OPTION,
			(char *) 0 );
}
/**************************************************************************
* fct_ibm_control
*	ACTION module for 'ibm_control'.
**************************************************************************/
fct_ibm_control( ibm_control_no, string )
	int	ibm_control_no;
	char	*string;
{
	char	outstring[ LEN_IBM_CONTROL + 1 ];
	int	auto_wrap_value;
	int	auto_scroll_value;
	int	winno;
	char	*mask;
	char	*value;
	int	reallen;
	
	winno = Outwin->number;
	strncpy( outstring, string, LEN_IBM_CONTROL );
	outstring[ LEN_IBM_CONTROL ] = '\0';
	/******************************************************************
	* If there is a force mask and value string for this control
	* then force the bits that are 1 in "mask" to the value of the
	* corresponding bits in "value".
	******************************************************************/
	mask = T_ibm_control_force_mask[ ibm_control_no ];
	value = T_ibm_control_force_value[ ibm_control_no ];
	if ( ( mask != (char *) 0 ) && ( value != (char *) 0 ) )
		ibm_control_force( outstring, mask, value );
	/******************************************************************
	* Perform the operation specified in "outstring" on the current
	* setting of the control group in "L_ibm_control".  If this is
	* an "or" or "and" etc that extends the currently known setting,
	* assume it was set to "T_ibm_control_default".
	******************************************************************/
	reallen = ibm_control_operation(
				L_ibm_control[ winno ][ ibm_control_no ],
				outstring,
				T_ibm_control_default[ ibm_control_no ] );
	/******************************************************************
	* See if this changed the auto wrap setting. < 0 means no.
	******************************************************************/
	auto_wrap_value = ibm_control_auto_wrap( ibm_control_no, outstring );
	if ( auto_wrap_value == 0 )
		win_se_auto_wrap_off();
	else if ( auto_wrap_value > 0 )
		win_se_auto_wrap_on();
	/******************************************************************
	* See if this changed the auto scroll setting. < 0 means no.
	******************************************************************/
	auto_scroll_value = ibm_control_auto_scroll( ibm_control_no, outstring);
	if ( auto_scroll_value == 0 )
		win_se_auto_scroll_off();
	else if ( auto_scroll_value > 0 )
		win_se_auto_scroll_on();
	/******************************************************************
	* When one window extends the known length of a control group, the
	* other windows must extend also.
	******************************************************************/
	ibm_control_set_other_defaults( ibm_control_no, reallen );
	/******************************************************************
	* Send the (possibly modified) command if current window.
	******************************************************************/
	if ( Outwin->onscreen )
	{
		term_ibm_control( ibm_control_no, outstring );
	}
}
/**************************************************************************
**************************************************************************/
ibm_control_operation( old, new, default_in )
	char	*old;
	char	*new;
	char	*default_in;
{
	char	c;
	char	*default_string;
	int	len;
	int	reallen;

	if ( default_in != (char *) 0 )
		default_string = default_in;
	else
		default_string = "";
	len = strlen( new );
	c = new[ len - 1 ];
	if ( ( c & 0x60 ) == 0x60 )
	{					/* operation */
		reallen = len - 1;
		if ( ( c & 0x1F ) == 1 )
		{				/* or */
			ibm_control_or( old, new, default_string );
		}
		else if ( ( c & 0x1F ) == 2 )
		{				/* and */
			ibm_control_and( old, new, default_string );
		}
		else
		{
			ibm_control_replace( old, new, default_string, 1 );
		}
	}
	else
	{					/* replace */
		reallen = len;
		ibm_control_replace( old, new, default_string, 0 );
	}
	return( reallen );
}
/**************************************************************************
*	extend "old" to length "len" using "default".
**************************************************************************/
ibm_control_extend( old, default_string, len )
	char	*old;
	char	*default_string;
	int	len;
{
	int	oldlen;
	int	deflen;

	oldlen = strlen( old );
	deflen = strlen( default_string );
	while ( oldlen < len )
	{
		if ( oldlen <= deflen )
			old[ oldlen ] = default_string[ oldlen ];
		else
			old[ oldlen ] = 0x20;
		oldlen++;
		old[ oldlen ] = '\0';
	}
}
/**************************************************************************
**************************************************************************/
ibm_control_or( old, new, default_string )
	char	*old;
	char	*new;
	char	*default_string;
{
	int	newlen;
	int	realnewlen;
	int	i;

	newlen = strlen( new );
	realnewlen = newlen - 1;
	ibm_control_extend( old, default_string, realnewlen );
	for( i = 0; i < realnewlen; i ++ )
	{
		old[ i ] &= 0x1F;
		old[ i ] |= ( new[ i ] & 0x1F );
		old[ i ] |= 0x20;
	}
}
/**************************************************************************
**************************************************************************/
ibm_control_and( old, new, default_string )
	char	*old;
	char	*new;
	char	*default_string;
{
	int	newlen;
	int	realnewlen;
	int	i;

	newlen = strlen( new );
	realnewlen = newlen - 1;
	ibm_control_extend( old, default_string, realnewlen );
	for( i = 0; i < realnewlen; i ++ )
	{
		old[ i ] &= ( new[ i ] & 0x1F );
		old[ i ] |= 0x20;
	}
}
/**************************************************************************
**************************************************************************/
ibm_control_replace( old, new, default_string, has_extra )
	char	*old;
	char	*new;
	char	*default_string;
	int	has_extra;
{
	int	newlen;
	int	realnewlen;
	int	i;

	newlen = strlen( new );
	if ( has_extra )
		realnewlen = newlen - 1;
	else
		realnewlen = newlen;
	for( i = 0; i < realnewlen; i ++ )
	{
		old[ i ] = ( new[ i ] & 0x1F ) | 0x20;
	}
}
/**************************************************************************
**************************************************************************/
ibm_control_force( new, mask, value )
	char	*new;
	char	*mask;
	char	*value;
{
	char	c;
	int	newlen;
	int	masklen;
	int	valuelen;
	int	len;
	int	i;
	int	cleanmask;
	int	cleanvalue;

	newlen = strlen( new );
	masklen = strlen( mask );
	valuelen = strlen( value );
	c = new[ newlen - 1 ];
	if ( ( c & 0x60 ) == 0x60 )
	{					/* operation */
		newlen--;
		len = newlen;
		if ( masklen < len )
			len = masklen;
		if ( valuelen < len )
			len = valuelen;
		if ( ( c & 0x1F ) == 1 )
		{				/* or */
			/* turn off: 1 in mask and 0 in value */
			for ( i = 0; i < len; i++ )
			{
				cleanmask = mask[ i ] & 0x001F;
				cleanvalue = value[ i ] & cleanmask;
				new[ i ] &= ( ( ~cleanmask ) | cleanvalue );
			}
		}
		else if ( ( c & 0x1F ) == 2 )
		{				/* and */
			/* turn on: 1 in mask and 1 in value */
			for ( i = 0; i < len; i++ )
			{
				cleanmask = mask[ i ] & 0x001F;
				cleanvalue = value[ i ] & cleanmask;
				new[ i ] |= cleanvalue;
			}
		}
		else
		{
			for ( i = 0; i < len; i++ )
			{
				cleanmask = mask[ i ] & 0x001F;
				cleanvalue = value[ i ] & cleanmask;
				new[ i ] &= ( ~cleanmask );
				new[ i ] |= cleanvalue;
			}
		}
	}
	else
	{					/* replace */
		len = newlen;
		if ( masklen < len )
			len = masklen;
		if ( valuelen < len )
			len = valuelen;
		for ( i = 0; i < len; i++ )
		{
			cleanmask = mask[ i ] & 0x001F;
			cleanvalue = value[ i ] & cleanmask;
			new[ i ] &= ( ~cleanmask );
			new[ i ] |= cleanvalue;
		}
	}
}
/**************************************************************************
**************************************************************************/
ibm_control_auto_wrap( ibm_control_no, new )
	int	ibm_control_no;
	char	*new;
{
	char	c;
	int	newlen;
	int	bit;
	int	realnewlen;
	int	newbit;

	if ( ibm_control_no != T_ibm_control_auto_wrap_number )
		return( -1 );
	newlen = strlen( new );
	c = new[ newlen - 1 ];
	if ( ( c & 0x60 ) == 0x60 )
	{					/* operation */
		realnewlen = newlen - 1;
		if ( realnewlen < T_ibm_control_auto_wrap_pos + 1 )
			return( -1 );
		bit = new[ T_ibm_control_auto_wrap_pos ] &
			   T_ibm_control_auto_wrap_mask;
		if ( ( c & 0x1F ) == 1 )
		{				/* or */
			if ( bit )
				newbit = 1;
			else
				return( -1 );
		}
		else if ( ( c & 0x1F ) == 2 )
		{				/* and */
			if ( bit )
				return( -1 );
			else
				newbit = 0; 
		}
		else
		{
			newbit = bit;
		}
	}
	else
	{					/* replace */
		if ( newlen < T_ibm_control_auto_wrap_pos + 1 )
			return( -1 );
		newbit = new[ T_ibm_control_auto_wrap_pos ] &
			   T_ibm_control_auto_wrap_mask;
	}
	if ( newbit )
	{
		if ( T_ibm_control_auto_wrap_sense )
			return( 1 );
		else
			return( 0 );
	}
	else
	{
		if ( T_ibm_control_auto_wrap_sense )
			return( 0 );
		else
			return( 1 );
	}
}
/**************************************************************************
**************************************************************************/
ibm_control_auto_scroll( ibm_control_no, new )
	int	ibm_control_no;
	char	*new;
{
	char	c;
	int	newlen;
	int	bit;
	int	realnewlen;
	int	newbit;

	if ( ibm_control_no != T_ibm_control_auto_scroll_number )
		return( -1 );
	newlen = strlen( new );
	c = new[ newlen - 1 ];
	if ( ( c & 0x60 ) == 0x60 )
	{					/* operation */
		realnewlen = newlen - 1;
		if ( realnewlen < T_ibm_control_auto_scroll_pos + 1 )
			return( -1 );
		bit = new[ T_ibm_control_auto_scroll_pos ] &
			   T_ibm_control_auto_scroll_mask;
		if ( ( c & 0x1F ) == 1 )
		{				/* or */
			if ( bit )
				newbit = 1;
			else
				return( -1 );
		}
		else if ( ( c & 0x1F ) == 2 )
		{				/* and */
			if ( bit )
				return( -1 );
			else
				newbit = 0; 
		}
		else
		{
			newbit = bit;
		}
	}
	else
	{					/* replace */
		if ( newlen < T_ibm_control_auto_scroll_pos + 1 )
			return( -1 );
		newbit = new[ T_ibm_control_auto_scroll_pos ] &
			   T_ibm_control_auto_scroll_mask;
	}
	if ( newbit )
	{
		if ( T_ibm_control_auto_scroll_sense )
			return( 1 );
		else
			return( 0 );
	}
	else
	{
		if ( T_ibm_control_auto_scroll_sense )
			return( 0 );
		else
			return( 1 );
	}
}
/**************************************************************************
* ibm_control_set_other_defaults
*	A window has output a control string "ibm_control_no" 
*	of length "reallen" on a window.
*	Extend other windows values using the default string if any.
**************************************************************************/
ibm_control_set_other_defaults( ibm_control_no, reallen )
	int	ibm_control_no;
	int	reallen;
{
	int	winno;
	int	len;
	char	*d;

	for ( winno = 0; winno < Wins; winno++ )
	{
		d = T_ibm_control_default[ ibm_control_no ];
		if ( d == (char *) 0 )
			continue;
		len = strlen( L_ibm_control[ winno ][ ibm_control_no ] );
		while ( len < reallen )
		{
			if ( len >= strlen( d ) )
				break;
			L_ibm_control[ winno ][ ibm_control_no ][ len ] =
								d[ len ];
			len++;
			L_ibm_control[ winno ][ ibm_control_no ][ len ] = '\0';
		}
	}
}
/**************************************************************************
* term_ibm_control
*	TERMINAL OUTPUT module for 'ibm_control'.
**************************************************************************/
term_ibm_control( ibm_control_no, string )
	int	ibm_control_no;
	char	*string;
{
	int	affcnt;			/* needs to be passed in ??? */
	char	*ibm_control_str;
	int	parm[ 2 ];		/* not used */
	char	*string_parm[ 1 ];
	char	*my_tparm();
	char	*p;
	int	auto_wrap_value;
	int	auto_scroll_value;

	affcnt = 1;
	if ( ibm_control_no < T_ibm_control_no )
	{
		ibm_control_str = T_ibm_control[ ibm_control_no ];
		if ( ibm_control_str != (char *) 0 )
		{
			string_parm[ 0 ] = string;
			p = my_tparm( ibm_control_str, parm, string_parm, -1 );
			my_tputs( p, affcnt );
			/**************************************************
			* See if this changed the auto wrap setting.
			* < 0 means no.
			**************************************************/
			auto_wrap_value = ibm_control_auto_wrap(
						ibm_control_no, string );
			if ( auto_wrap_value == 0 )
				term_se_auto_wrap_off();
			else if ( auto_wrap_value > 0 )
				term_se_auto_wrap_on();
			/**************************************************
			* See if this changed the auto scroll setting.
			**************************************************/
			auto_scroll_value = ibm_control_auto_scroll(
						ibm_control_no, string);
			if ( auto_scroll_value == 0 )
				term_se_auto_scroll_off();
			else if ( auto_scroll_value > 0 )
				term_se_auto_scroll_on();
			/**************************************************
			* Record effect on terminal.
			**************************************************/
			ibm_control_operation(
				M_ibm_control[ ibm_control_no ],
				string,
				T_ibm_control_default[ ibm_control_no ] );
		}
	}
}
/**************************************************************************
**************************************************************************/
t_sync_ibm_control_modes_off()
{
	if ( T_ibm_control_no < 0 )
		return;
	t_sync_ibm_control_auto_wrap_on();
}
t_sync_ibm_control_modes_on( winno )
	int	winno;
{
	if ( T_ibm_control_no < 0 )
		return;
	t_sync_ibm_control_auto_wrap( winno );
}
#define IBM_CONTROL_SCREEN 0
#define IBM_CONTROL_INPUT 1
t_sync_ibm_control_modes_screen_attribute()
{
	if ( T_ibm_control_no < 0 )
		return;
	t_sync_ibm_control( Outwin->number, IBM_CONTROL_SCREEN );
}
t_sync_ibm_control_input_modes()
{
	if ( T_ibm_control_no < 0 )
		return;
	t_sync_ibm_control( Outwin->number, IBM_CONTROL_INPUT );
}
t_sync_ibm_control( winno, kind )
	int	winno;
	int	kind;
{
	int	i;
	char	*want;

	for ( i = 0; i < T_ibm_control_no; i++ )
	{
		if ( T_ibm_control_kind[ i ] == kind )
		{
			want = L_ibm_control[ winno ][ i ];
			if ( strcmp( want, M_ibm_control[ i ] ) != 0 )
			{
				term_ibm_control( i, want );
			}
		}
	}
}
t_sync_ibm_control_auto_wrap_on()
{
	if ( T_ibm_control_auto_wrap_number < 0 )
		return;
	if ( T_ibm_control_auto_wrap_on_out != (char *) 0 )
	{
		if ( M_auto_wrap_on == 0 )
		{
			term_ibm_control( T_ibm_control_auto_wrap_number,
					  T_ibm_control_auto_wrap_on_out );
		}
	}
}
t_sync_ibm_control_auto_wrap( winno )
	int	winno;
{
	char	*p;

	if ( T_ibm_control_auto_wrap_number < 0 )
		return;
	winno = Outwin->number;
	if ( ( Outwin->auto_wrap_on == 0 ) && M_auto_wrap_on )
	{
		p = L_ibm_control[ winno ][ T_ibm_control_auto_wrap_number ];
		if ( ( p != (char *)0 ) && ( *p != 0 ) )
		{
			term_ibm_control( T_ibm_control_auto_wrap_number,
					  p );
		}
		else if ( T_ibm_control_auto_wrap_off_out != (char *) 0 )
		{
			term_ibm_control( T_ibm_control_auto_wrap_number,
					  T_ibm_control_auto_wrap_off_out );
		}
	}
}
/**************************************************************************
* extra_ibm_control
*	TERMINAL DESCRIPTION PARSER module for 'ibm_control'.
**************************************************************************/
char *dec_encode();
extra_ibm_control( buff, string, attr_on_string, attr_off_string ) 
	char	*buff;
	char	*string;
	char	*attr_on_string;
	char	*attr_off_string;
{
	char	*encoded;

	if ( strcmp( buff, "ibm_control" ) == 0 )
	{
		if ( T_ibm_control_no < MAX_IBM_CONTROL )
		{
			encoded = dec_encode( string );
			T_ibm_control[ T_ibm_control_no ] = encoded;
			inst_ibm_control( encoded, T_ibm_control_no );
			if ( attr_on_string != (char *) 0 )
				T_ibm_control_kind[ T_ibm_control_no ] =
							atoi( attr_on_string );
			T_ibm_control_no++;
		}
		else
		{
			printf( "Too many ibm_control. Max = %d\n", 
				MAX_IBM_CONTROL );
		}
	}
	else if ( strcmp( buff, "ibm_control_auto_wrap" ) == 0 )
	{
		if ( T_ibm_control_no <= 0 )
			printf( "ibm_control_auto_wrap before ibm_control.\n" );
		else
		{
			T_ibm_control_auto_wrap_number = T_ibm_control_no - 1;
			if ( attr_on_string == (char *) 0 )
				T_ibm_control_auto_wrap_pos = 0;
			else
			{
				T_ibm_control_auto_wrap_pos =
							atoi( attr_on_string );
			}
			if ( attr_off_string == (char *) 0 )
				T_ibm_control_auto_wrap_sense = 1;
			else
			{
				T_ibm_control_auto_wrap_sense =
							atoi( attr_off_string );
			}
			T_ibm_control_auto_wrap_mask = *dec_encode( string );
		}
	}
	else if ( strcmp( buff, "ibm_control_auto_scroll" ) == 0 )
	{
		if ( T_ibm_control_no <= 0 )
			printf(
			    "ibm_control_auto_scroll before ibm_control.\n" );
		else
		{
			T_ibm_control_auto_scroll_number = T_ibm_control_no - 1;
			if ( attr_on_string == (char *) 0 )
				T_ibm_control_auto_scroll_pos = 0;
			else
			{
				T_ibm_control_auto_scroll_pos =
							atoi( attr_on_string );
			}
			if ( attr_off_string == (char *) 0 )
				T_ibm_control_auto_scroll_sense = 1;
			else
			{
				T_ibm_control_auto_scroll_sense =
							atoi( attr_off_string );
			}
			T_ibm_control_auto_scroll_mask = *dec_encode( string );
		}
	}
	else if ( strcmp( buff, "ibm_control_auto_wrap_on_out" ) == 0 )
	{
		T_ibm_control_auto_wrap_on_out = dec_encode( string );
	}
	else if ( strcmp( buff, "ibm_control_auto_wrap_off_out" ) == 0 )
	{
		T_ibm_control_auto_wrap_off_out = dec_encode( string );
	}
	else if ( strcmp( buff, "ibm_control_default" ) == 0 )
	{
		if ( T_ibm_control_no <= 0 )
			printf( "ibm_control_default before ibm_control.\n");
		else
		{
			T_ibm_control_default[ T_ibm_control_no - 1 ] =
							dec_encode( string );
		}
	}
	else if ( strcmp( buff, "ibm_control_force_mask" ) == 0 )
	{
		if ( T_ibm_control_no <= 0 )
			printf( "ibm_control_force_mask before ibm_control.\n");
		else
		{
			T_ibm_control_force_mask[ T_ibm_control_no - 1 ] =
							dec_encode( string );
		}
	}
	else if ( strcmp( buff, "ibm_control_force_value" ) == 0 )
	{
		if ( T_ibm_control_no <= 0 )
			printf("ibm_control_force_value before ibm_control.\n");
		else
		{
			T_ibm_control_force_value[ T_ibm_control_no - 1 ] =
							dec_encode( string );
		}
	}
	else
	{
		return( 0 );		/* no match */
	}
	return( 1 );
}
