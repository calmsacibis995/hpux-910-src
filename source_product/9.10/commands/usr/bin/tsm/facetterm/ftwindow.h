/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: ftwindow.h,v 70.1 92/03/09 15:44:20 ssa Exp $ */
/* ftwindow.h */

#include "ftchar.h"
#define MAX_COLS		132
#define MAX_ROWS		72

#define MAX_CHARACTER_SETS	4

#define MAX_STATUS_LINE_CHARS	80

#define MAX_PASTE_END_OF_LINE	10

/* note 8 initializers in statusline.c for MAX_STATUS_LINE_LABEL */

#define MAX_STATUS_LINE_LABEL		 8
#define MAX_STATUS_LINE_LABEL_CHARS	 9

typedef struct t_function_key_struct T_FUNCTION_KEY;

typedef struct 
{
	int		number;
	unsigned char	onscreen;
	unsigned char	in_terminal;
	unsigned char	row_changed_all;
	unsigned char	col_changed_all;
	unsigned char	fullscreen;
	unsigned char	insert_mode;
	unsigned char	auto_wrap_on;
	unsigned char	auto_scroll_on;
	unsigned char	write_protect_on;
	unsigned char	appl_keypad_mode_on;
	unsigned char	cursor_key_mode_on;
	unsigned char	keypad_xmit;
	unsigned char	cursor_on;
	unsigned char	status_on;
	unsigned char	status_type_after_status_line;
	unsigned char	status_line_labels_after_status_line;
	unsigned char	xenl;
	unsigned char	real_xenl;
	unsigned char	origin_mode;
	unsigned char	save_cursor_origin_mode;
	unsigned char	columns_wide_mode_on;
	unsigned char	line_attribute_current;
	unsigned char	pc_mode_on;
	unsigned char	rows_changeno;
	unsigned char	extra_data_row;
	unsigned char	control_8_bit_on;
	unsigned char	terminal_mode;
	unsigned char	terminal_mode_is_scan_code;
	unsigned char	graphics_8_bit;
	unsigned char	transparent_print_on;
	unsigned char	graph_mode_on;
	unsigned char	graph_screen_on;
	unsigned char	onstatus;
	unsigned char	personality;
	FTCHAR		ftattrs;
	FTCHAR		save_cursor_ftattrs;
	unsigned short	character_set;
	unsigned short	save_cursor_character_set;
	unsigned short	hp_charset_select;	/* 2397 */
	unsigned short	cursor_type;
	unsigned short	status_type;
	unsigned short	cols;
	unsigned short	col_right;
	unsigned short	col_right_line;
	unsigned short	select_character_set[ MAX_CHARACTER_SETS ];
	int		display_rows;
	int		display_row_bottom;
	int		row;
	int		col;
	int		save_cursor_row;
	int		save_cursor_col;
	int		win_top_row;
	int		win_bot_row;
	int		buff_top_row;
	int		buff_bot_row;
	int		csr_buff_top_row;
	int		csr_buff_bot_row;
	int		memory_lock_row;
	int		set_margin_left;
	int		set_margin_right;
	int		wrap_margin_left;
	int		wrap_margin_right;
	long		mode;
	int		(*special_function)();
	long		*special_ptr;
	char		status_line[		MAX_STATUS_LINE_CHARS + 1 ];
	char		status_line_label[	MAX_STATUS_LINE_LABEL ]
					 [ MAX_STATUS_LINE_LABEL_CHARS + 1 ];
	T_FUNCTION_KEY	*function_key_ptr;
	unsigned char	line_attribute[		MAX_ROWS ];
	unsigned char	row_changed[		MAX_ROWS ];
	unsigned char	col_changed[		MAX_COLS ];
	char		paste_end_of_line[	MAX_PASTE_END_OF_LINE + 1 ];
	FTCHAR		*ftchars[		MAX_ROWS ];
	FTCHAR		**scroll_memory_ftchars;
} FT_WIN;

extern FT_WIN	*Curwin;        /* current window */
extern FT_WIN	*Outwin;	/* current output window */
extern FT_WIN	*Wininfo[];
extern int	O_pe;		/* current output window (Outwin) personality */
