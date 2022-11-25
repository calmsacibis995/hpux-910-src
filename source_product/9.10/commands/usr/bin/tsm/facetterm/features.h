/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: features.h,v 70.1 92/03/09 15:42:20 ssa Exp $ */
#include "person.h"
#include "features_p.h"
extern int	F_xon_xoff;
extern int	F_force_CS8;
extern int	F_no_clear_ISTRIP;
extern int	F_graphics_8_bit;
extern int	sF_graphics_escape_control;
extern int	sF_graphics_escape_delete;
extern int	sF_delete_is_a_character;
extern int	F_eat_newline_glitch[ MAX_PERSONALITY ];
extern int	F_real_eat_newline_glitch;
extern int	F_auto_left_margin;		/* bs wraps to prev line */
extern int	sF_insdel_line_move_col0;
extern int	sF_scroll_reverse_move_col0;
extern int	sF_scroll_could_be_cursor_only;
extern int	sF_use_csr;
extern int	F_no_split_screen;
extern FTCHAR	F_character_set_attributes;
extern int	F_columns_wide_clears_screen;
extern int	F_columns_wide_switch_resets_scroll_region;
extern int	F_columns_wide_switch_reload_scroll_region;
extern int	F_cursor_up_at_home_wraps_ll;
extern int	sF_insert_line_needs_clear_glitch;
extern int	sF_insert_line_sets_attributes;
extern int	F_columns_wide_mode_on_default;
extern int	F_auto_scroll_off_wraps_top;
extern int	F_columns_wide_clears_onstatus;
extern int	F_rows_change_does_clear_screen;
extern int	sF_hp_attribute;
extern int	sF_memory_below;
extern int	sF_has_scroll_memory;
extern int	F_tilde_glitch;
extern int	F_allow_tabs;
extern int	F_auto_wrap_on_default;
