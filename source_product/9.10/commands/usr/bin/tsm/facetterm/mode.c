/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: mode.c,v 70.1 92/03/09 15:45:33 ssa Exp $ */
/**************************************************************************
* mode.c
*	a 32 bit mode word that maintains terminal modes per window.
*	mode:		sequences that affect a single bit;
*	multi_mode:	sequences that affect several bits as a field;
*	parm_mode:	sequences that affect several single bits based on
*			parameters in the sequence.
**************************************************************************/
#include <stdio.h>
#include "mode_p.h"
#include "ftproc.h"
#include "ftwindow.h"
#include "features.h"
#include "decode.h"
#include "options.h"

long	M_mode = 0;

#define MAX_MODE	32

typedef struct t_mode_struct T_MODE;
struct t_mode_struct
{
	T_MODE			*next;
	char			*t_mode;
	long			t_mode_bits_on;
	long			t_mode_bits_off;
};
T_MODE	*sT_mode_ptr = { (T_MODE *) 0 };

/******************************************************************
* Strings corresponding to each of the mode bits to turn the
* mode on and to turn the mode off.
******************************************************************/
T_MODE	*sT_mode_ptr_on[ MAX_MODE ] = {(T_MODE *) 0};
T_MODE	*sT_mode_ptr_off[ MAX_MODE ] = {(T_MODE *) 0};

typedef struct t_multi_mode_struct T_MULTI_MODE;
struct t_multi_mode_struct
{
	T_MULTI_MODE		*next;
	char			*t_multi_mode;
	long			t_multi_mode_mask;
	long			t_multi_mode_value;
};
T_MULTI_MODE	*sT_multi_mode_ptr = { (T_MULTI_MODE *) 0 };

char	*sT_parm_mode_on = { NULL };
char	*sT_parm_mode_off =  { NULL };
/* parm_mode_val-00002000-00000000=16 */
/* parm_mode_val-00000010-00000000=?16 */
#define PM_NORMAL 0
#define PM_PRIVATE  1

typedef struct t_parm_mode_val_struct T_PARM_MODE_VAL;
struct t_parm_mode_val_struct
{
	T_PARM_MODE_VAL		*next;
	int			t_val;
	int			t_sel;
	long			t_parm_mode_val_bits_on;
	long			t_parm_mode_val_bits_off;
};
T_PARM_MODE_VAL	*sT_parm_mode_val_ptr = { (T_PARM_MODE_VAL *) 0 };

int	sF_parm_mode_private_propogates = { 0 };

struct MODE_STRINGS
{
	char	mode_name[9];
	long	mode_value;
};
struct MODE_STRINGS Mode_strings[] =
{
	{ "IGNORE..",	0x80000000 },
	{ "PASS....",	0x40000000 },
	{ "NOTIMP..",	0x20000000 },
	{ "COLSWIDE",	0x08000000 },
	{ "AUTOWRAP",	0x04000000 },
	{ "INSERTON",	0x02000000 },
	{ "CURSORON",	0x00800000 },
	{ "APPKEYON",	0x00400000 },
	{ "CURKEYON",	0x00200000 },
	{ "KEYPADXM",	0x00100000 },
	{ "PASSCURR",	0x00080000 },
	{ "PASSSPER",	0x00040000 },
	{ "",		0x00000000 }
};
long	F_ignore_mode_bit = 			0x80000000;
long	F_pass_mode_bit = 			0x40000000;
long	F_notimp_mode_bit = 			0x20000000;
long	F_columns_wide_mode_on_mode_bit = 	0x08000000;
long	F_auto_wrap_on_mode_bit = 		0x04000000;
long	F_insert_mode_on_mode_bit = 		0x02000000;
long	F_cursor_on_mode_bit = 			0x00800000;
long	F_appl_keypad_mode_on_mode_bit = 	0x00400000;
long	F_cursor_key_mode_on_mode_bit = 	0x00200000;
long	F_keypad_xmit_mode_bit = 		0x00100000;
long	F_pass_current_mode_bit = 		0x00080000;
long	F_pass_same_personality_mode_bit =	0x00040000;

/**************************************************************************
* dec_mode
*	DECODE module for 'mode'.
**************************************************************************/
/*ARGSUSED*/
dec_mode( unused, t_mode_ptr )
	int	unused;
	T_MODE	*t_mode_ptr;
{
	fct_mode( t_mode_ptr );
}
/**************************************************************************
* inst_mode
*	INSTALL module for 'mode'.
**************************************************************************/
inst_mode( str, t_mode_ptr )
	char	*str;
	T_MODE	*t_mode_ptr;
{
	dec_install( "mode", (UNCHAR *) str, dec_mode, 
		0, 0,
		(char *) t_mode_ptr );
}
/**************************************************************************
* fct_mode
*	ACTION module for 'mode'.
**************************************************************************/
fct_mode( t_mode_ptr )
	T_MODE	*t_mode_ptr;
{
	long	bits_off;
	long	bits_on;

	bits_off = t_mode_ptr->t_mode_bits_off;
	bits_on =  t_mode_ptr->t_mode_bits_on;
	fct_mode_bits_change( bits_on, bits_off );
	if ( Outwin->onscreen )
		t_sync_mode( Outwin->mode );
}
/**************************************************************************
* term_mode
*	TERMINAL OUTPUT module for 'mode'.
**************************************************************************/
term_mode( t_mode_ptr )
	T_MODE	*t_mode_ptr;
{
	char	*modestr;

	modestr = t_mode_ptr->t_mode;
	if ( modestr != NULL )
	{
		term_tputs( modestr );
		M_mode &= (~(t_mode_ptr->t_mode_bits_off));
		M_mode |=    t_mode_ptr->t_mode_bits_on;
	}
}
/**************************************************************************
* term_se_mode
*	An event has occurred that had the side effect of
*	turning on the mode bits "mode_bits_on" and
*	turning off the mode bits "mode_bits_off" in the terminal.
**************************************************************************/
term_se_mode( mode_bits_on, mode_bits_off )
	long	mode_bits_on;
	long	mode_bits_off;
{
	M_mode &= (~mode_bits_off);
	M_mode |=    mode_bits_on;
}
/**************************************************************************
* win_se_mode
*	An event has occurred that had the side effect of
*	turning on the mode bits "mode_bits_on" and
*	turning off the mode bits "mode_bits_off" on the window.
**************************************************************************/
win_se_mode( mode_bits_on, mode_bits_off )
	long	mode_bits_on;
	long	mode_bits_off;
{
	Outwin->mode &= (~mode_bits_off);
	Outwin->mode |=    mode_bits_on;
}
/**************************************************************************
* dec_multi_mode
*	DECODE module for 'multi_mode'.
**************************************************************************/
/*ARGSUSED*/
dec_multi_mode( unused, t_multi_mode_ptr )
	int		unused;
	T_MULTI_MODE	*t_multi_mode_ptr;
{
	fct_multi_mode( t_multi_mode_ptr );
}
/**************************************************************************
* inst_multi_mode
*	INSTALL module for 'multi_mode'.
**************************************************************************/
inst_multi_mode( str, t_multi_mode_ptr )
	char	*str;
	T_MULTI_MODE	*t_multi_mode_ptr;
{
	dec_install( "multi_mode", (UNCHAR *) str, dec_multi_mode, 
		0, 0,
		(char *) t_multi_mode_ptr );
}
/**************************************************************************
* fct_multi_mode
*	ACTION module for 'multi_mode'.
**************************************************************************/
fct_multi_mode( t_multi_mode_ptr )
	T_MULTI_MODE	*t_multi_mode_ptr;
{
	Outwin->mode &= (~(t_multi_mode_ptr->t_multi_mode_mask));
	Outwin->mode |=    t_multi_mode_ptr->t_multi_mode_value;
	if ( Outwin->onscreen )
		t_sync_mode( Outwin->mode );
}
/**************************************************************************
* term_multi_mode
*	TERMINAL OUTPUT module for 'multi_mode'.
**************************************************************************/
term_multi_mode( t_multi_mode_ptr )
	T_MULTI_MODE	*t_multi_mode_ptr;
{
	char		*multi_modestr;

	multi_modestr = t_multi_mode_ptr->t_multi_mode;
	if ( multi_modestr != NULL )
	{
		term_tputs( multi_modestr );
		M_mode &= (~(t_multi_mode_ptr->t_multi_mode_mask));
		M_mode |=    t_multi_mode_ptr->t_multi_mode_value;
	}
}
/**************************************************************************
* dec_parm_mode
*	DECODE module for 'parm_mode'.
*	"parm_mode_on" indicates whether the parameters are turned on or off.
*	The parameters are in "parms" with "parms_valid" indicating which ones;
*	and in "string_parms" with "strings_parms_valid" indicating which ones.
**************************************************************************/
/*ARGSUSED*/
dec_parm_mode( parm_mode_on, parm_ptr, 
	       parms_valid, parms, string_parms, string_parms_valid )
	int	parm_mode_on;
	char	*parm_ptr;
	int	parms_valid;
	int	parms[];
	char	*string_parms[];
	int	string_parms_valid;
{
	fct_parm_mode( parm_mode_on, parms_valid, parms, string_parms,
							string_parms_valid );
}
/**************************************************************************
* inst_parm_mode
*	INSTALL module for 'parm_mode'.
**************************************************************************/
inst_parm_mode( str, parm_mode_on )
	char	*str;
	int	parm_mode_on;
{
	dec_install( "parm_mode", (UNCHAR *) str, dec_parm_mode, parm_mode_on,
							CURSOR_OPTION,
							(char *) 0 );
}
/**************************************************************************
* fct_parm_mode
*	ACTION module for 'parm_mode'.
**************************************************************************/
fct_parm_mode( parm_mode_on, parms_valid, parms, string_parms,
							string_parms_valid )
	int	parm_mode_on;
	int	parms_valid;
	int	parms[];
	char	*string_parms[];
	int	string_parms_valid;
{
	int	i;
	int	mask;
	int	sel;
	char	buff[ 80 ];
	long	bits_on;
	long	bits_off;
	int	force_private;
	char	*private;
	int	parm_mode_val;
	T_PARM_MODE_VAL	*t_parm_mode_val_ptr;

	force_private = 0;
	for ( i = 0, mask = 1;
	      parms_valid & mask;
	      i++, mask <<= 1 )
	{
		if ( force_private )
			sel = PM_PRIVATE;
		else if ( string_parms_valid & mask )
		{
			sel = PM_PRIVATE;
			if ( oF_parm_mode_private_propogates )
				force_private = 1;
						/* private is string_parms[ i ]
						 * or string that forced private
					 	 */
			private = string_parms[ i ];
		}
		else
			sel = PM_NORMAL;
		bits_on = 0;
		bits_off = 0;
		parm_mode_val = parms[ i ];
		for( t_parm_mode_val_ptr = oT_parm_mode_val_ptr;
		     t_parm_mode_val_ptr != ( T_PARM_MODE_VAL *) 0;
		     t_parm_mode_val_ptr = t_parm_mode_val_ptr->next )
		{
			if (  ( t_parm_mode_val_ptr->t_val == parm_mode_val )
			   && ( t_parm_mode_val_ptr->t_sel == sel ) )
			{
			    if ( parm_mode_on )
			    {
				bits_on = 
				  t_parm_mode_val_ptr->t_parm_mode_val_bits_on;
				bits_off = 
				  t_parm_mode_val_ptr->t_parm_mode_val_bits_off;
			    }
			    else
			    {
				bits_off = 
				  t_parm_mode_val_ptr->t_parm_mode_val_bits_on;
				bits_on =  
				  t_parm_mode_val_ptr->t_parm_mode_val_bits_off;
			    }
			}
		}
		if ( (bits_on == 0) && (bits_off == 0 ) )
		{
			if ( Opt_error_ignore )
			{
			}
			else if ( Opt_error_pass )
			{
				if ( parm_mode_on )
					term_pass_parm_mode_on(  
						parm_mode_val, sel, private );
				else
					term_pass_parm_mode_off(
						parm_mode_val, sel, private );
			}
			else
			{
				term_beep();
				sprintf( buff, "parm_mode_val_unknown=%d-%d-%d",
					 parm_mode_val, sel, parm_mode_on );
				error_record_msg( buff );
			}
			continue;
		}
		if ( bits_on & F_pass_mode_bit )
		{
			if ( parm_mode_on )
				term_pass_parm_mode_on(  parm_mode_val, sel,
							 private );
			else
				term_pass_parm_mode_off( parm_mode_val, sel,
							 private );
			bits_on &= (~F_pass_mode_bit);
		}
		if ( bits_on & F_pass_current_mode_bit )
		{
			if ( outwin_is_curwin() )
			{
			    if ( parm_mode_on )
				term_pass_parm_mode_on(  parm_mode_val, sel,
							 private );
			    else
				term_pass_parm_mode_off( parm_mode_val, sel,
							 private );
			}
			bits_on &= (~F_pass_current_mode_bit);
		}
		if ( bits_on & F_pass_same_personality_mode_bit )
		{
			if ( Outwin->personality == M_pe )
			{
			    if ( parm_mode_on )
				term_pass_parm_mode_on(  parm_mode_val, sel,
							 private );
			    else
				term_pass_parm_mode_off( parm_mode_val, sel,
							 private );
			}
			bits_on &= (~F_pass_same_personality_mode_bit);
		}
		if ( bits_off & F_pass_mode_bit )
		{
			bits_off &= (~F_pass_mode_bit);
		}
		if ( bits_off & F_pass_current_mode_bit )
		{
			bits_off &= (~F_pass_current_mode_bit);
		}
		if ( bits_off & F_pass_same_personality_mode_bit )
		{
			bits_off &= (~F_pass_same_personality_mode_bit);
		}
		if ( bits_on & F_ignore_mode_bit )
		{
			bits_on &= (~F_ignore_mode_bit);
		}
		if ( bits_off & F_ignore_mode_bit )
		{
			bits_off &= (~F_ignore_mode_bit);
		}
		if ( bits_on & F_notimp_mode_bit )
		{
			bits_on &= (~F_notimp_mode_bit);
			if ( Opt_error_ignore )
			{
			}
			else if ( Opt_error_pass )
			{
			}
			else
			{
				term_beep();
				if ( parm_mode_on )
					sprintf( buff,
					 "parm_mode_val_on_not_imp=%d-%d",
					 parm_mode_val, sel );
				else
					sprintf( buff, 
					 "parm_mode_val_off_not_imp=%d-%d",
					 parm_mode_val, sel );
				error_record_msg( buff );
			}
		}
		if ( bits_off & F_notimp_mode_bit )
		{
			bits_off &= (~F_notimp_mode_bit);
		}
		fct_mode_bits_change( bits_on, bits_off );
	}
	if ( Outwin->onscreen )
		t_sync_mode( Outwin->mode );
}
/**************************************************************************
* term_pass_parm_mode_on
*	Output to the terminal parm mode settings designated as pass in
*	a parm mode on sequence.
*	The parameter is in "parm".
*	"sel" is a flag that if set to private, causes "parm" to be
*	prefixed by "private";
*	E.G.    \E[7h    and    \E[?7h (private)
**************************************************************************/
term_pass_parm_mode_on( parm, sel, private )
	int	parm;
	int	sel;
	char	*private;
{
	int	parms[ 2 ];			/* 1 used - must be >= 2 - %i */
	char	*string_parm[ 1 ];
	char	*my_tparm();
	char	*p;

	parms[ 0 ] = parm;
	if ( sel == PM_PRIVATE )
		string_parm[ 0 ] = private;
	else
		string_parm[ 0 ] = "";
	if ( mT_parm_mode_on != NULL )
	{
		p = my_tparm( mT_parm_mode_on, parms, string_parm, -1 );
		term_tputs( p );
	}
}
/**************************************************************************
* term_pass_parm_mode_off
*	Output to the terminal parm mode settings designated as pass in
*	a parm mode off sequence.
*	The parameter is in "parm".
*	"sel" is a flag that if set to private, causes "parm" to be
*	prefixed by "private";
*	E.G.    \E[7l    and    \E[?7l (private)
**************************************************************************/
term_pass_parm_mode_off( parm, sel, private )
	int	parm;
	int	sel;
	char	*private;
{
	int	parms[ 2 ];			/* 1 used - must be >= 2 - %i */
	char	*string_parm[ 1 ];
	char	*my_tparm();
	char	*p;

	parms[ 0 ] = parm;
	if ( sel == PM_PRIVATE )
		string_parm[ 0 ] = private;
	else
		string_parm[ 0 ] = "";
	if ( mT_parm_mode_off != NULL )
	{
		p = my_tparm( mT_parm_mode_off, parms, string_parm, -1 );
		term_tputs( p );
	}
}
/**************************************************************************
* t_sync_mode
*	Syncronize the terminal, if necessary, to the mode "mode".
*	Try 'mode=' and 'multi_mode='.
*	Parm modes are also present in those two also.
**************************************************************************/
t_sync_mode( mode )
	long	mode;
{
	int	i;
	long	change;
	long	bitchange;
	char	buff[ 80 ];
	T_MODE		*t_mode_ptr;
	T_MULTI_MODE	*t_multi_mode_ptr;

	change = mode ^ M_mode;
	if ( change == 0 )
		return;
	for ( i = 0; i < MAX_MODE; i++ )
	{
		bitchange = change & ( 1 << i );
		if ( bitchange )
		{
			if ( bitchange & mode )
				t_mode_ptr = mT_mode_ptr_on[ i ];
			else
				t_mode_ptr = mT_mode_ptr_off[ i ];
			if ( t_mode_ptr != (T_MODE *)0 )
				term_mode( t_mode_ptr );
		}
	}
	for ( t_multi_mode_ptr = mT_multi_mode_ptr; 
	      t_multi_mode_ptr != (T_MULTI_MODE *)0;
	      t_multi_mode_ptr = t_multi_mode_ptr->next )
	{
		change = mode ^ M_mode;
		if ( change == 0 )
			return;
		if ( change & t_multi_mode_ptr->t_multi_mode_mask )
		{
			if ( ( mode & t_multi_mode_ptr->t_multi_mode_mask ) == 
				      t_multi_mode_ptr->t_multi_mode_value )
			{
				term_multi_mode( t_multi_mode_ptr );
			}
		}
	}
	change = mode ^ M_mode;
	if ( change != 0 )
	{
		term_beep();
		sprintf( buff, "mode_sync_failed=0x%lx-0x%lx",
			 mode, M_mode );
		error_record_msg( buff );
	}
}
/**************************************************************************
* t_set_mode
*	Output to the terminal sequences necessary to put it in the modes
*	corresponding to the bits in "mode" for all bits that are one in
*	the bit mask "change".
*	The others are assumed to be ok.
**************************************************************************/
t_set_mode( mode, change )
	long	mode;
	long	change;
{
	int	i;
	long	bitchange;
	T_MODE		*t_mode_ptr;
	char	buff[ 80 ];
	T_MULTI_MODE	*t_multi_mode_ptr;

	if ( change == 0 )
		return;
	for ( i = 0; i < MAX_MODE; i++ )
	{
		bitchange = change & ( 1 << i );
		if ( bitchange )
		{
			if ( bitchange & mode )
				t_mode_ptr = mT_mode_ptr_on[ i ];
			else
				t_mode_ptr = mT_mode_ptr_off[ i ];
			if ( t_mode_ptr != (T_MODE *) 0 )
				term_mode( t_mode_ptr );
		}
	}
	for ( t_multi_mode_ptr = mT_multi_mode_ptr; 
	      t_multi_mode_ptr != (T_MULTI_MODE *)0;
	      t_multi_mode_ptr = t_multi_mode_ptr->next )
	{
		if ( change & t_multi_mode_ptr->t_multi_mode_mask )
		{
			if ( ( mode & t_multi_mode_ptr->t_multi_mode_mask ) == 
				      t_multi_mode_ptr->t_multi_mode_value )
			{
				term_multi_mode( t_multi_mode_ptr );
			}
		}
	}
	change = mode ^ M_mode;
	if ( ( mode & change )!= ( M_mode & change ) )
	{
		term_beep();
		sprintf( buff, "mode_set_failed=0x%lx-0x%lx",
			 ( mode & change ), ( M_mode & change ) );
		error_record_msg( buff );
	}
}

/**************************************************************************
* fct_mode_bits_change
*	ACTION module for the mode constructs.
*	The sequence has turned the mode bits "bits_on" on 
*	and the mode bits "bits_off" off.
*	Sort out the bits that are not really mode bits but modes
*	that are handled by their own modules and we are setting
*	and clearing several at a time with a parm mode.
**************************************************************************/
fct_mode_bits_change( bits_on, bits_off )
	long	bits_on;
	long	bits_off;
{
	if ( bits_on & F_auto_wrap_on_mode_bit )
	{
		fct_auto_wrap_on();
		bits_on &= (~F_auto_wrap_on_mode_bit);
		bits_off &= (~F_auto_wrap_on_mode_bit);
	}
	else if ( bits_off & F_auto_wrap_on_mode_bit )
	{
		fct_auto_wrap_off();
		bits_off &= (~F_auto_wrap_on_mode_bit);
	}
	if ( bits_on & F_insert_mode_on_mode_bit )
	{
		fct_enter_insert_mode();
		bits_on &= (~F_insert_mode_on_mode_bit);
		bits_off &= (~F_insert_mode_on_mode_bit);
	}
	else if ( bits_off & F_insert_mode_on_mode_bit )
	{
		fct_exit_insert_mode();
		bits_off &= (~F_insert_mode_on_mode_bit);
	}
	if ( bits_on & F_appl_keypad_mode_on_mode_bit )
	{
		fct_enter_appl_keypad_mode();
		bits_on &= (~F_appl_keypad_mode_on_mode_bit);
		bits_off &= (~F_appl_keypad_mode_on_mode_bit);
	}
	else if ( bits_off & F_appl_keypad_mode_on_mode_bit )
	{
		fct_exit_appl_keypad_mode();
		bits_off &= (~F_appl_keypad_mode_on_mode_bit);
	}
	if ( bits_on & F_cursor_key_mode_on_mode_bit )
	{
		fct_enter_cursor_key_mode();
		bits_on &= (~F_cursor_key_mode_on_mode_bit);
		bits_off &= (~F_cursor_key_mode_on_mode_bit);
	}
	else if ( bits_off & F_cursor_key_mode_on_mode_bit )
	{
		fct_exit_cursor_key_mode();
		bits_off &= (~F_cursor_key_mode_on_mode_bit);
	}
	if ( bits_on & F_keypad_xmit_mode_bit )
	{
		fct_keypad_xmit();
		bits_on &= (~F_keypad_xmit_mode_bit);
		bits_off &= (~F_keypad_xmit_mode_bit);
	}
	else if ( bits_off & F_keypad_xmit_mode_bit )
	{
		fct_keypad_local();
		bits_off &= (~F_keypad_xmit_mode_bit);
	}
	if ( bits_on & F_columns_wide_mode_on_mode_bit )
	{
		fct_columns_wide_on();
		bits_on &= (~F_columns_wide_mode_on_mode_bit);
		bits_off &= (~F_columns_wide_mode_on_mode_bit);
	}
	else if ( bits_off & F_columns_wide_mode_on_mode_bit )
	{
		fct_columns_wide_off();
		bits_off &= (~F_columns_wide_mode_on_mode_bit);
	}
	if ( bits_on & F_cursor_on_mode_bit )
	{
		fct_cursor_on();
		bits_on &= (~F_cursor_on_mode_bit);
		bits_off &= (~F_cursor_on_mode_bit);
	}
	else if ( bits_off & F_cursor_on_mode_bit )
	{
		fct_cursor_on();
		bits_off &= (~F_cursor_on_mode_bit);
	}
	Outwin->mode &= (~bits_off);
	Outwin->mode |=   bits_on;
	if ( Outwin->onscreen )
		t_set_mode( Outwin->mode, ( bits_on | bits_off ) );
}
/****************************************************************************/
#include "linklast.h"
char *dec_encode();
/**************************************************************************
* extra_mode
*	TERMINAL DESCRIPTION PARSER module for '...mode...'.
**************************************************************************/
extra_mode( buff, string, attr_on_string, attr_off_string ) 
	char	*buff;
	char	*string;
	char	*attr_on_string;
	char	*attr_off_string;
{
	char	*encoded;
	long	mode_encode();
	T_MULTI_MODE	*t_multi_mode_ptr;
	T_PARM_MODE_VAL	*t_parm_mode_val_ptr;
	long		*mymalloc();

	if ( strcmp( buff, "mode" ) == 0 )
	{
		test_mode( string, attr_on_string, attr_off_string );
	}
	else if ( strcmp( buff, "multi_mode" ) == 0 )
	{
		t_multi_mode_ptr = 
			(T_MULTI_MODE *) mymalloc( sizeof( T_MULTI_MODE ), 
					  "multi_mode" );
		t_multi_mode_ptr->next = (T_MULTI_MODE *) 0;
		link_last( (T_STRUCT *)   t_multi_mode_ptr,
			   (T_STRUCT *) &xT_multi_mode_ptr );
		encoded = dec_encode( string );
		t_multi_mode_ptr->t_multi_mode = encoded;
		inst_multi_mode( encoded, t_multi_mode_ptr );
		t_multi_mode_ptr->t_multi_mode_mask = 
						mode_encode( attr_on_string );
		t_multi_mode_ptr->t_multi_mode_value =
						mode_encode( attr_off_string );
	}
	else if ( strcmp( buff, "parm_mode_val" ) == 0 )
	{
		int	val;
		int	sel;

		if ( (*string >= '0') && (*string <= '9') )
		{
			val = atoi( string );
			sel = PM_NORMAL;
		}
		else
		{
			val = atoi( &string[ 1 ] );
			sel = PM_PRIVATE;
		}
		t_parm_mode_val_ptr = 
		    (T_PARM_MODE_VAL *) mymalloc( sizeof( T_PARM_MODE_VAL ), 
					  "parm_mode_val" );
		t_parm_mode_val_ptr->next = (T_PARM_MODE_VAL *) 0;
		link_last( (T_STRUCT *)   t_parm_mode_val_ptr,
			   (T_STRUCT *) &xT_parm_mode_val_ptr );
		t_parm_mode_val_ptr->t_val = val;
		t_parm_mode_val_ptr->t_sel = sel;
		t_parm_mode_val_ptr->t_parm_mode_val_bits_on =
					mode_encode( attr_on_string );
		t_parm_mode_val_ptr->t_parm_mode_val_bits_off =
					mode_encode( attr_off_string );
	}
	else if ( strcmp( buff, "parm_mode_on" ) == 0 )
	{
		do_parm_mode( string, 1 );
	}
	else if ( strcmp( buff, "parm_mode_off" ) == 0 )
	{
		do_parm_mode( string, 0 );
	}
	else if ( strcmp( buff, "parm_mode_private_propogates" ) == 0 )
	{
		xF_parm_mode_private_propogates =
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "auto_wrap_mode_bit" ) == 0 )
	{
		F_auto_wrap_on_mode_bit = mode_encode( attr_on_string );
	}
	else if ( strcmp( buff, "insert_mode_on_mode_bit" ) == 0 )
	{
		F_insert_mode_on_mode_bit = mode_encode( attr_on_string );
	}
	else if ( strcmp( buff, "appl_keypad_mode_on_mode_bit" ) == 0 )
	{
		F_appl_keypad_mode_on_mode_bit = mode_encode( attr_on_string );
	}
	else if ( strcmp( buff, "cursor_key_mode_on_mode_bit" ) == 0 )
	{
		F_cursor_key_mode_on_mode_bit = mode_encode( attr_on_string );
	}
	else if ( strcmp( buff, "keypad_xmit_mode_bit" ) == 0 )
	{
		F_keypad_xmit_mode_bit = mode_encode( attr_on_string );
	}
	else if ( strcmp( buff, "columns_wide_mode_on_mode_bit" ) == 0 )
	{
		F_columns_wide_mode_on_mode_bit = mode_encode( attr_on_string );
	}
	else if ( strcmp( buff, "cursor_on_mode_bit" ) == 0 )
	{
		F_cursor_on_mode_bit = mode_encode( attr_on_string );
	}
	else if ( strcmp( buff, "pass_mode_bit" ) == 0 )
	{
		F_pass_mode_bit = mode_encode( attr_on_string );
	}
	else if ( strcmp( buff, "ignore_mode_bit" ) == 0 )
	{
		F_ignore_mode_bit = mode_encode( attr_on_string );
	}
	else if ( strcmp( buff, "notimp_mode_bit" ) == 0 )
	{
		F_notimp_mode_bit = mode_encode( attr_on_string );
	}
	else
	{
		return( 0 );		/* no match */
	}
	return( 1 );
}
/**************************************************************************
* mode_encode
*	Encode the mode word in "string".
*	This could be a hex representation of a 32 bit word or
*	a symbolic name from the mode_name array.
*	Return the encoded result or 0 on failure.
**************************************************************************/
long
mode_encode( string )
	char	*string;
{
	long	result;
	char	*s;
	unsigned char	c;
	struct MODE_STRINGS *p;

	result = 0;
	if ( string == NULL )
		return( 0 );
	for ( p = Mode_strings; p->mode_name[ 0 ] != '\0'; p++ )
	{
		if ( strcmp( string, p->mode_name ) == 0 )
			return( p->mode_value );
	}
	s = string;
	while ( (c = *s++) != '\0' )
	{
		result <<= 4;
		if ( c >= '0' && c <= '9' )
			result |= ( c - '0' );
		else if ( c >= 'A' && c <= 'F' )
			result |= ( 10 + c - 'A' );
		else
		{
			printf( "Did not encode mode %s\n", string );
			return( (long) 0 );
		}
	}
	return( result );
}
/**************************************************************************
* test_mode
*	Detailed parsing for 'mode='.
**************************************************************************/
test_mode( string, attr_on_string, attr_off_string )
	char	*string;
	char	*attr_on_string;
	char	*attr_off_string;
{
	char	*encoded;
	long	mode_bits_on;
	long	mode_bits_off;
	T_MODE	*t_mode_ptr;
	long	*mymalloc();

	t_mode_ptr = 
		(T_MODE *) mymalloc( sizeof( T_MODE ), 
				  "mode" );
	t_mode_ptr->next = (T_MODE *) 0;
	link_last( (T_STRUCT *)   t_mode_ptr, 
		   (T_STRUCT *) &xT_mode_ptr );

	encoded = dec_encode( string );
	t_mode_ptr->t_mode = encoded;
	inst_mode( encoded, t_mode_ptr );
	mode_bits_on = mode_encode( attr_on_string );
	t_mode_ptr->t_mode_bits_on = mode_bits_on;
	mode_bits_off = mode_encode( attr_off_string );
	t_mode_ptr->t_mode_bits_off = mode_bits_off;
	test_mode_single( t_mode_ptr, mode_bits_on, mode_bits_off );
}
/**************************************************************************
* test_mode_single
*	See if the 'mode=' construct turns a single mode on or off.
*	If so then it is useful for changing the terminal.
*	Multiple effect sequences must also have a single effect to
*	match them so multiple effect ones are ignored.
**************************************************************************/
test_mode_single( t_mode_ptr, bits_on, bits_off )
	T_MODE	*t_mode_ptr;
	long	bits_on;
	long	bits_off;
{
	int	i;

	if ( bits_on == 0 )
	{
		for ( i = 0; i < MAX_MODE; i++ )
		{
			if ( bits_off == ( 1 << i ) )
			{
				xT_mode_ptr_off[ i ] = t_mode_ptr;
				return;
			}
		}
	}
	else if ( bits_off == 0 )
	{
		for ( i = 0; i < MAX_MODE; i++ )
		{
			if ( bits_on == ( 1 << i ) )
			{
				xT_mode_ptr_on[ i ] = t_mode_ptr;
				return;
			}
		}
	}
}
/**************************************************************************
* do_parm_mode
*	Detailed parsing for 'parm_mode_on=' and 'parm_mode_off='.
*	"parm_mode_on" specifies which was seen.
*	Install all the possibilities implied by the sequence syntax.
**************************************************************************/
#define MAX_STORE_LEN 256
do_parm_mode( string, parm_mode_on )
	char	*string;
	int	parm_mode_on;
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

	if ( parse_for_percent_M( string, front, back, &special, &repeat ) )
	{
	    strcpy( newstring, front );
	    strcat( newstring, "%s1%p1%d" );
	    strcat( newstring, back );
	    encoded = dec_encode( newstring );
	    if ( parm_mode_on )
		xT_parm_mode_on = encoded;
	    else
		xT_parm_mode_off = encoded;
	    for ( i = 1; i <= repeat; i++ )
	    {
		for ( j = i; j >= 0; j-- )
		{
						/* add \E[ */
			strcpy( newstring, front );
						/* add %p1%d;%p2%d;%p3%d; */
			for ( k = 1; k <= j; k++ )
			{
				if ( k < 10 )
					sprintf( buff, "%%p%d%%d", k );
				else
					strcpy( buff, "%pA%d" );
				strcat( newstring, buff );
				if ( k != i )
					strcat( newstring, ";" );
			}
						/* add %C4?%p4%d */
			if ( k <= i )
			{
				if ( k < 10 )
					sprintf( buff, "%cC%d%c%%p%d%%d",
						'%', k, special, k );
				else
					sprintf( buff, "%%CA%c%%pA%%d",
						special );
				strcat( newstring, buff );
				if ( k < i )
					strcat( newstring, ";" );
				k++;
			}
						/* add %m5?;%m6?;%m7? */
			for(	; k <= i; k++ )
			{
				if ( k < 10 )
					sprintf( buff, "%%m%d%c", k, special );
				else
					sprintf( buff, "%%mA%c", special );
				strcat( newstring, buff );
				if ( k != i )
					strcat( newstring, ";" );
			}
						/* add m */
			strcat( newstring, back );
			if ( strlen( newstring ) < MAX_STORE_LEN )
			{
				encoded = temp_encode( newstring, storage );
				inst_parm_mode( encoded, parm_mode_on );
			}
			else
			{
				printf( "parm_mode string is too big %d\n",
					strlen( newstring ) );
				printf( "'%s'\n", newstring );
			}
		}
	    }
	}
	else
	{
		encoded = dec_encode( string );
		if ( parm_mode_on )
			xT_parm_mode_on = encoded;
		else
			xT_parm_mode_off = encoded;
		inst_parm_mode( encoded, parm_mode_on );
	}
}
/**************************************************************************
* parse_for_percent_M
*	The parm mode strings support the syntax:
*		% number M special 
*	which means up to 'number' semicolon separated sequences of decimal
*	numbers with the optional character 'special' in front of them.
*	If the string "string" contains such a construct,
*	return the characters of "string" before the % in "front",
*	the characters after "special" in "back",
*	the special character in "special",
*	the number in the integer pointed to by "p_repeat",
*	and a 1.
*	Otherwise return 0.
**************************************************************************/
parse_for_percent_M( string, front, back, special, p_repeat )
	char	*string;
	char	*front;
	char	*back;
	char	*special;
	int	*p_repeat;
{
	char	*s;
	char	*f;

	s = string;
	f = front;
	while ( *s != '\0' )
	{
		if (  ( s[ 0 ] == '%' )
		   && ( s[ 1 ] >= '0' ) && ( s[ 1 ] <= '9' )
		   && ( s[ 2 ] == 'M' ) 
		   && ( s[ 3 ] != '\0' ) )
		{
			*f++ = '\0';
			strcpy( back, &s[ 4 ] );
			*special = s[ 3 ];
			*p_repeat = atoi( &s[ 1 ] );
			return( 1 );
		}
		else if (  ( s[ 0 ] == '%' )
		        && ( s[ 1 ] == '1' )
		        && ( s[ 2 ] == '0' )
		        && ( s[ 3 ] == 'M' ) 
			&& ( s[ 4 ] != '\0' ) )
		{
			*f++ = '\0';
			strcpy( back, &s[ 5 ] );
			*special = s[ 4 ];
			*p_repeat = atoi( &s[ 1 ] );
			return( 1 );
		}
		*f++ = *s++;
	}
	return( 0 );
}
