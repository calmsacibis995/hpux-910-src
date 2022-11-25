/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: text.c,v 70.1 92/03/09 15:47:35 ssa Exp $ */
/**************************************************************************
* text.c
*	Allow override of the text strings presented to the user - sender.
**************************************************************************/
#include "foreign.h"
/* ============================= capture.c ================== */
extern char	 *Map_get_command_capture_active;
extern char	*Text_get_command_capture_active;
extern char	 *Map_get_command_capture_inactive;
extern char	*Text_get_command_capture_inactive;
extern char	*Text_name_input_capture_file;
extern char	 *Map_get_command_keystroke_active;
extern char	*Text_get_command_keystroke_active;
extern char	 *Map_get_command_keystroke_inactive;
extern char	*Text_get_command_keystroke_inactive;
extern char	*Text_name_input_keystroke_file;
extern char	*Text_name_input_keystroke_play;
/* ============================= commonsend.c =============== */
extern char	*Text_number_of_windows;
extern char	*Text_facet_line_available;
extern char	*Text_all_facet_busy;
extern char	*Text_driver_not_installed;
extern char	*Text_assign_facet_busy;
extern char	*Text_assign_facet_exist;
extern char	*Text_window_status_error;
extern char	*Text_assign_windows_open;
extern char	*Text_facet_line_windows_open;
extern char	*Text_cannot_open_dev_facet;
extern char	*Text_facet_activating;
extern char	*Text_sender_wait_close;
extern char	*Text_facet_attempt_kill;
extern char	*Text_facet_attempt_list;
extern char	*Text_sender_term_wait_close;
extern char	*Text_sender_term;
extern char	*Text_use_activate_to_run;
/* ============================= control8.c ================= */
extern char	*Text_terminal_modes;
/* ============================= facetterm.c ================ */
extern char	*Text_window_is_active;
extern char	*Text_window_is_idle;
extern char	*Text_user_number;
extern char	*Text_too_many_users;
/* ============================= ftcommand.c ================ */
extern char	*Text_facetterm_window_label;
extern char	*Text_window_printer_mode;
extern char	 *Map_get_command_redirect;
extern char	*Text_get_command_redirect;
extern char	*Prmt_get_command_quit_active;
extern char	 *Map_get_command_quit_active;
extern char	*Text_get_command_quit_active;
extern char	*Text_get_command_quit_idle;
extern char	*Text_name_input_run_program;
extern char	*Text_name_input_key_file;
extern char	*Text_name_input_global_key_file;
extern char	*Text_name_input_paste_eol_type;
extern char	 *Map_get_command_extra_commands;
extern char	*Text_get_command_extra_commands;
extern char	 *Map_get_command_control_chars;
extern char	*Text_get_command_control_chars;
extern char	*Text_name_input_window_title;
extern char	 *Map_get_command_mapped;
extern char	*Text_get_command_mapped;
extern char	 *Map_get_command_paste_commands;
extern char	*Text_get_command_paste_commands;
extern char	 *Map_window_mode_prompt;
extern char	*Text_window_mode_prompt;
extern char	*Text_window_mode_prompt_bottom;
extern char	*Text_window_mode_prompt_top;
extern char	*Text_window_mode_prompt_full;
extern char	 *Map_window_mode_prompt_popup;
extern char	*Text_window_mode_prompt_popup;
extern char	*Text_window_mode_prompt_default;
extern char	*Text_block_cut_and_paste_first;
extern char	*Text_block_cut_and_paste_second;
extern char	*Text_stream_cut_and_paste_first;
extern char	*Text_stream_cut_and_paste_second;
extern char	*Text_wrap_cut_and_paste_first;
extern char	*Text_wrap_cut_and_paste_second;
extern char	*Text_cut_and_paste_paste;
extern char	 *Map_cut_and_paste;
extern char	*Text_cut_and_paste;
extern char	 *Map_name_input;
extern char	*Text_name_input;
extern char	*Text_get_window_idle_window;
extern char	*Text_get_window_windows_window;
extern char	 *Map_get_window;
extern char	*Text_get_window;
extern char	*Text_get_window_next_active;
extern char	*Text_get_window_menu;
extern char	*Text_get_window_none;
extern char	*Prmt_get_command_window;
extern char	 *Map_get_command_window_monitor;
extern char	*Text_get_command_window_monitor;
extern char	 *Map_get_command_window_invisible;
extern char	*Text_get_command_window_invisible;
extern char	 *Map_get_command_window_notify;
extern char	*Text_get_command_window_notify;
extern char	 *Map_get_command_window_transparent;
extern char	*Text_get_command_window_transparent;
extern char	 *Map_get_command_window_blocked;
extern char	*Text_get_command_window_blocked;
extern char	 *Map_get_command_window_printer;
extern char	*Text_get_command_window_printer;
extern char	 *Map_get_command_send_hangup;
extern char	*Text_get_command_send_hangup;
extern char	*Prmt_get_command_screen_saver;
extern char	 *Map_get_command_screen_saver;
extern char	*Text_get_command_screen_saver;
extern char	 *Map_get_mode_input;
extern char	*Text_get_mode_input;
extern char	*Text_get_command;
extern char	*Text_full_switch_window;
extern char	*Text_full_refresh_window;
extern char	*Text_name_input_mapped_key;
extern char	*Text_name_input_mapped_key_unmap;
extern char	*Text_name_input_paste_script;
extern char	*Text_name_input_paste_filename;
extern char	*Text_screen_saver;
extern char	*Text_screen_lock;
extern char	*Text_printer_mode_on;
extern char	*Text_printer_mode_off;
extern char	*Text_name_input_run_raw_tty;
extern char	*Text_facetterm_run_raw_tty;
extern char	*Text_facetterm_resuming;
extern char	 *Map_get_command_screen_lock;
extern char	*Text_get_command_screen_lock;
extern char	*Text_name_input_password;
extern char	*Text_name_input_password2;
extern char	*Text_name_input_unpassword;
/* ============================= ftextra.c ================== */
extern char	*Text_cannot_open_terminal_desc;
/* ============================= ftterm.c =================== */
extern char	*Text_term_not_set;
extern char	*Text_term_type_is;
extern char	*Text_reading_terminal_desc;
extern char	*Text_menu_hotkey_is_control;
extern char	*Text_menu_hotkey_is_char;
extern char	*Text_hotkey_disabled;
extern char	*Text_hotkey_is_control;
extern char	*Text_hotkey_is_char;
extern char	*Text_press_return_to_exit;
extern char	*Text_press_return_to_continue;
extern char	*Text_split_screen_disabled;
/* ============================= print.c ==================== */
extern char	*Text_transparent_print;
extern char	*Text_transparent_print_off;
extern char	*Text_transparent_print_on;
/* ============================= tpnotify.c================== */
extern char	*Text_printer_mode_terminated;
/* ============================= wincommon.c ================ */
extern char	*Text_sender_removing_utmp;
/* ============================= activecom.c ================ */
extern char	*Text_printer_program_name;

/* ============================= ../reg/checkauth.in ======== */

/* ============================= end of list ================ */
char	*Text_end_of_list = "";

TEXTNAMES Textnames[] =
{
/* ============================= capture.c ================== */
     "in_get_command_capture_active",	 &Map_get_command_capture_active,
	"get_command_capture_active",	&Text_get_command_capture_active,
     "in_get_command_capture_inactive",	 &Map_get_command_capture_inactive,
	"get_command_capture_inactive",	&Text_get_command_capture_inactive,
	"name_input_capture_file",	&Text_name_input_capture_file,
     "in_get_command_keystroke_active",	 &Map_get_command_keystroke_active,
	"get_command_keystroke_active",	&Text_get_command_keystroke_active,
     "in_get_command_keystroke_inactive",  &Map_get_command_keystroke_inactive,
	"get_command_keystroke_inactive", &Text_get_command_keystroke_inactive,
	"name_input_keystroke_file",	&Text_name_input_keystroke_file,
	"name_input_keystroke_play",	&Text_name_input_keystroke_play,
/* ============================= commonsend.c =============== */
	"number_of_windows",		&Text_number_of_windows,
	"facet_line_available",		&Text_facet_line_available,
	"all_facet_busy",		&Text_all_facet_busy,
	"driver_not_installed",		&Text_driver_not_installed,
	"assign_facet_busy",		&Text_assign_facet_busy,
	"assign_facet_exist",		&Text_assign_facet_exist,
	"window_status_error",		&Text_window_status_error,
	"assign_windows_open",		&Text_assign_windows_open,
	"facet_line_windows_open",	&Text_facet_line_windows_open,
	"cannot_open_dev_facet",	&Text_cannot_open_dev_facet,
	"facet_activating",		&Text_facet_activating,
	"sender_wait_close",		&Text_sender_wait_close,
	"facet_attempt_kill",		&Text_facet_attempt_kill,
	"facet_attempt_list",		&Text_facet_attempt_list,
	"sender_term_wait_close",	&Text_sender_term_wait_close,
	"sender_term",			&Text_sender_term,
	"use_activate_to_run",		&Text_use_activate_to_run,
/* ============================= control8.c ================= */
	"terminal_modes",		&Text_terminal_modes,
/* ============================= facetterm.c ================ */
	"window_is_active",		&Text_window_is_active,
	"window_is_idle",		&Text_window_is_idle,
	"user_number",			&Text_user_number,
	"too_many_users",		&Text_too_many_users,
/* ============================= ftcommand.c ================ */
	"facetterm_window_label",	&Text_facetterm_window_label,
	"window_printer_mode",		&Text_window_printer_mode,
     "in_get_command_redirect",		 &Map_get_command_redirect,
	"get_command_redirect",		&Text_get_command_redirect,
 "prompt_get_command_quit_active",	&Prmt_get_command_quit_active,
     "in_get_command_quit_active",	 &Map_get_command_quit_active,
	"get_command_quit_active",	&Text_get_command_quit_active,
	"get_command_quit_idle",	&Text_get_command_quit_idle,
	"name_input_run_program",	&Text_name_input_run_program,
	"name_input_key_file",		&Text_name_input_key_file,
	"name_input_global_key_file",	&Text_name_input_global_key_file,
	"name_input_paste_eol_type",	&Text_name_input_paste_eol_type,
     "in_get_command_extra_commands",	 &Map_get_command_extra_commands,
	"get_command_extra_commands",	&Text_get_command_extra_commands,
     "in_get_command_control_chars",	 &Map_get_command_control_chars,
	"get_command_control_chars",	&Text_get_command_control_chars,
	"name_input_window_title",	&Text_name_input_window_title,
     "in_get_command_mapped",		 &Map_get_command_mapped,
	"get_command_mapped",		&Text_get_command_mapped,
     "in_get_command_paste_commands",	 &Map_get_command_paste_commands,
	"get_command_paste_commands",	&Text_get_command_paste_commands,
     "in_window_mode_prompt",		 &Map_window_mode_prompt,
	"window_mode_prompt",		&Text_window_mode_prompt,
	"window_mode_prompt_bottom",	&Text_window_mode_prompt_bottom,
	"window_mode_prompt_top",	&Text_window_mode_prompt_top,
	"window_mode_prompt_full",	&Text_window_mode_prompt_full,
     "in_window_mode_prompt_popup",	 &Map_window_mode_prompt_popup,
	"window_mode_prompt_popup",	&Text_window_mode_prompt_popup,
	"window_mode_prompt_default",	&Text_window_mode_prompt_default,
	"block_cut_and_paste_first",	&Text_block_cut_and_paste_first,
	"block_cut_and_paste_second",	&Text_block_cut_and_paste_second,
	"stream_cut_and_paste_first",	&Text_stream_cut_and_paste_first,
	"stream_cut_and_paste_second",	&Text_stream_cut_and_paste_second,
	"wrap_cut_and_paste_first",	&Text_wrap_cut_and_paste_first,
	"wrap_cut_and_paste_second",	&Text_wrap_cut_and_paste_second,
	"cut_and_paste_paste",		&Text_cut_and_paste_paste,
     "in_cut_and_paste",		 &Map_cut_and_paste,
	"cut_and_paste",		&Text_cut_and_paste,
     "in_name_input",			 &Map_name_input,
	"name_input",			&Text_name_input,
	"get_window_idle_window",	&Text_get_window_idle_window,
	"get_window_windows_window",	&Text_get_window_windows_window,
     "in_get_window",			 &Map_get_window,
	"get_window",			&Text_get_window,
	"get_window_next_active",	&Text_get_window_next_active,
	"get_window_menu",		&Text_get_window_menu,
	"get_window_none",		&Text_get_window_none,
 "prompt_get_command_window",		&Prmt_get_command_window,
     "in_get_command_window_monitor",	 &Map_get_command_window_monitor,
	"get_command_window_monitor",	&Text_get_command_window_monitor,
     "in_get_command_window_invisible",	 &Map_get_command_window_invisible,
	"get_command_window_invisible",	&Text_get_command_window_invisible,
     "in_get_command_window_notify",	 &Map_get_command_window_notify,
	"get_command_window_notify",	&Text_get_command_window_notify,
     "in_get_command_window_transparent",  &Map_get_command_window_transparent,
	"get_command_window_transparent", &Text_get_command_window_transparent,
     "in_get_command_window_blocked",	 &Map_get_command_window_blocked,
	"get_command_window_blocked",	&Text_get_command_window_blocked,
     "in_get_command_window_printer",	 &Map_get_command_window_printer,
	"get_command_window_printer",	&Text_get_command_window_printer,
     "in_get_command_send_hangup",	 &Map_get_command_send_hangup,
	"get_command_send_hangup",	&Text_get_command_send_hangup,
 "prompt_get_command_screen_saver",	&Prmt_get_command_screen_saver,
     "in_get_command_screen_saver",	 &Map_get_command_screen_saver,
	"get_command_screen_saver",	&Text_get_command_screen_saver,
     "in_get_mode_input",		 &Map_get_mode_input,
	"get_mode_input",		&Text_get_mode_input,
	"get_command",			&Text_get_command,
	"full_switch_window",		&Text_full_switch_window,
	"full_refresh_window",		&Text_full_refresh_window,
	"name_input_mapped_key",	&Text_name_input_mapped_key,
	"name_input_mapped_key_unmap",	&Text_name_input_mapped_key_unmap,
	"name_input_paste_script",	&Text_name_input_paste_script,
	"name_input_paste_filename",	&Text_name_input_paste_filename,
	"screen_saver",			&Text_screen_saver,
	"screen_lock",			&Text_screen_lock,
	"printer_mode_on",		&Text_printer_mode_on,
	"printer_mode_off",		&Text_printer_mode_off,
	"name_input_run_raw_tty",	&Text_name_input_run_raw_tty,
	"facetterm_run_raw_tty",	&Text_facetterm_run_raw_tty,
	"facetterm_resuming",		&Text_facetterm_resuming,
     "in_get_command_screen_lock",	 &Map_get_command_screen_lock,
	"get_command_screen_lock",	&Text_get_command_screen_lock,
	"name_input_password",		&Text_name_input_password,
	"name_input_password2",		&Text_name_input_password2,
	"name_input_unpassword",	&Text_name_input_unpassword,
/* ============================= ftextra.c ================== */
	"cannot_open_terminal_desc",	&Text_cannot_open_terminal_desc,
/* ============================= ftterm.c =================== */
	"term_not_set",			&Text_term_not_set,
	"term_type_is",			&Text_term_type_is,
	"reading_terminal_desc",	&Text_reading_terminal_desc,
	"menu_hotkey_is_control",	&Text_menu_hotkey_is_control,
	"menu_hotkey_is_char",		&Text_menu_hotkey_is_char,
	"hotkey_disabled",		&Text_hotkey_disabled,
	"hotkey_is_control",		&Text_hotkey_is_control,
	"hotkey_is_char",		&Text_hotkey_is_char,
	"press_return_to_exit",		&Text_press_return_to_exit,
	"press_return_to_continue",	&Text_press_return_to_continue,
	"split_screen_disabled",	&Text_split_screen_disabled,
/* ============================= print.c ==================== */
	"transparent_print",		&Text_transparent_print,
	"transparent_print_off",	&Text_transparent_print_off,
	"transparent_print_on",		&Text_transparent_print_on,
/* ============================= tpnotify.c ================= */
	"printer_mode_terminated",	&Text_printer_mode_terminated,
/* ============================= wincommon.c ================ */
	"sender_removing_utmp",		&Text_sender_removing_utmp,
/* ============================= activecom.c ================ */
	"printer_program_name",		&Text_printer_program_name,

/* ============================= ../reg/checkauth.in ======== */

/* ============================= end of list ================ */
	"",				&Text_end_of_list
};
