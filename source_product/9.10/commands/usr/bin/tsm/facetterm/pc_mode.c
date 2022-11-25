/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: pc_mode.c,v 66.6 90/10/18 09:25:52 kb Exp $ */
/**************************************************************************
* pc_mode.c
*	Support VPIX scan code terminals.
**************************************************************************/
/****************************************************************************/
char *dec_encode();
/**************************************************************************
* extra_pc_mode
*	TERMINAL DESCRIPTION PARSER module for 'pc_mode'.
**************************************************************************/
/*ARGSUSED*/
extra_pc_mode( buff, string, attr_on_string, attr_off_string ) 
	char	*buff;
	char	*string;
	char	*attr_on_string;
	char	*attr_off_string;
{
	long	mode_encode();

	if ( strcmp( buff, "pc_mode_on" ) == 0 )
	{
	}
	else if ( strcmp( buff, "pc_mode_off" ) == 0 )
	{
	}
	else if ( strcmp( buff, "out_pc_mode_on_1" ) == 0 )
	{
	}
	else if ( strcmp( buff, "out_pc_mode_on_2" ) == 0 )
	{
	}
	else if ( strcmp( buff, "out_pc_mode_on_3" ) == 0 )
	{
	}
	else if ( strcmp( buff, "out_pc_mode_off_1" ) == 0 )
	{
	}
	else if ( strcmp( buff, "out_pc_mode_off_2" ) == 0 )
	{
	}
	else if ( strcmp( buff, "out_pc_mode_off_3" ) == 0 )
	{
	}
	else if ( strcmp( buff, "pc_mode_se_on_switch_only" ) == 0 )
	{
	}
	else if ( strcmp( buff, "pc_mode_does_clear_screen" ) == 0 )
	{
	}
	else if ( strcmp( buff, "pc_mode_on_turns_auto_wrap_off" ) == 0 )
	{
	}
	else if ( strcmp( buff, "pc_mode_off_turns_auto_wrap_on" ) == 0 )
	{
	}
	else if ( strcmp( buff, "pc_mode_on_switch_page_number_0" ) == 0 )
	{
	}
	else if ( strcmp( buff, "pc_mode_off_switch_page_number_0" ) == 0 )
	{
	}
	else if ( strcmp( buff, "pc_auto_wrap_on" ) == 0 )
	{
	}
	else if ( strcmp( buff, "pc_auto_wrap_off" ) == 0 )
	{
	}
	else if ( strcmp( buff, "pc_mode_sets_cursor_on" ) == 0 )
	{
	}
	else if ( strcmp( buff, "pc_mode_resets_insert_mode" ) == 0 )
	{
	}
	else if ( strcmp( buff, "pc_mode_resets_appl_keypad_mode" ) == 0 )
	{
	}
	else if ( strcmp( buff, "pc_mode_resets_keypad_xmit" ) == 0 )
	{
	}
	else if ( strcmp( buff, "pc_mode_resets_scroll_region" ) == 0 )
	{
	}
	else if ( strcmp( buff, "pc_mode_resets_character_set" ) == 0 )
	{
	}
	else if ( strcmp( buff, "pc_mode_resets_attributes" ) == 0 )
	{
	}
	else if ( strcmp( buff, "pc_mode_resets_save_cursor" ) == 0 )
	{
	}
	else if ( strcmp( buff, "pc_mode_resets_origin_mode" ) == 0 )
	{
	}
	else if ( strcmp( buff, "pc_mode_sets_mode" ) == 0 )
	{
	}
	else if ( strcmp( buff, "pc_mode_defaults_function_keys" ) == 0 )
	{
	}
	else if ( strcmp( buff, "pc_mode_kd_scancode_driver" ) == 0 )
	{
	}
	else
	{
		return( 0 );		/* no match */
	}
	return( 1 );
}
