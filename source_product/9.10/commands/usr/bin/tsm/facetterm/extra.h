/*****************************************************************************
** Copyright (c) 1986 - 1991 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: extra.h,v 70.1 92/03/09 15:41:57 ssa Exp $ */
#include "person.h"
#include "extra_p.h"
extern char	*T_carriage_return;
extern char	*T_cursor_address[ MAX_PERSONALITY ];
extern char	*T_cursor_address_wide[ MAX_PERSONALITY ];
extern char	*sT_row_address;
extern char	*sT_column_address;
extern char	*sT_column_address_parm_down_cursor;
extern char	*sT_column_address_parm_up_cursor;
extern char	*sT_new_line;
extern char	*sT_cursor_home;
extern char	*sT_cursor_up;
extern char	*sT_parm_up_cursor;
extern char	*sT_cursor_down;
extern char	*sT_parm_down_cursor;
extern char	*sT_cursor_right;
extern char	*sT_parm_right_cursor;
extern char	*sT_cursor_left;
extern char	*sT_parm_left_cursor;
extern char	*sT_cursor_to_ll;
extern char	*sT_out_clear_screen;
extern char	*sT_clear_all;
extern char	*sT_clear_all_w_attr;
extern char	*sT_clr_eol;
extern char	*sT_out_clr_eol;
extern char	*sT_clr_eos;
extern char	*sT_out_clr_eos;
extern char	*sT_clr_eol_w_attr;
extern char	*sT_clr_eos_w_attr;
extern char	*sT_clr_eol_unprotected;
extern char	*sT_clr_fld_unprotected;
extern char	*sT_clr_eos_unprotected;
extern char	*sT_clr_eol_unprotected_w_attr;
extern char	*sT_clr_fld_unprotected_w_attr;
extern char	*sT_clr_eos_unprotected_w_attr;
extern char	*sT_insert_character;
extern char	*sT_parm_ich;
extern char	*sT_enter_insert_mode;
extern char	*sT_exit_insert_mode;
extern char	*sT_delete_character;
extern char	*sT_parm_delete_character;
extern char	*sT_insert_line;
extern char	*sT_out_insert_line;
extern char	*sT_parm_insert_line;
extern char	*sT_out_parm_insert_line;
extern char	*sT_delete_line;
extern char	*sT_out_delete_line;
extern char	*sT_parm_delete_line;
extern char	*sT_out_parm_delete_line;
extern char	*sT_cursor_home_down;
extern char	*sT_scroll_forward;
extern char	*sT_parm_index;
extern char	*sT_scroll_reverse;
extern char	*sT_parm_rindex;
extern char	*T_flash_screen;
extern char	*sT_change_scroll_region;
extern char	*sT_memory_lock;
extern char	*sT_memory_unlock;
extern char	*sT_save_cursor;
extern char	*sT_restore_cursor;



extern char	*sT_insert_padding;
extern char	*T_nomagic;
#define MAX_MAGIC	40
extern int	T_magicno;
extern char	*T_magic[ MAX_MAGIC ];

extern char	*sT_auto_wrap_on;
extern char	*sT_auto_wrap_off;

extern char	*T_columns_wide_on;
extern char	*T_columns_wide_off;

typedef struct t_ca_mode_struct T_CA_MODE;
struct t_ca_mode_struct
{
	T_CA_MODE		*next;
	char			*t_ca_mode;
};

extern T_CA_MODE	*T_enter_ca_mode_ptr;
extern T_CA_MODE	*T_enter_ca_mode_2_ptr;
extern T_CA_MODE	*T_exit_ca_mode_ptr;
extern T_CA_MODE	*T_exit_ca_mode_2_ptr;
