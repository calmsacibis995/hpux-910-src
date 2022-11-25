# FacetTerm  vwpt  (80 column only)  description file 6/20/91
#    ADDS Viewpoint 60 terminal in vp/60 mode..
## Copyright (c) Structured Software Solutions, Inc. 1991.  All Rights Reserved.
## @(#) $Header: vwpt.FS,v 70.1 92/03/09 16:52:54 ssa Exp $
xon_xoff
columns=80
lines=24
### graphics_escape_control
### graphics_escape_delete
### columns=80
### columns_wide=132
### columns_wide_mode_on_default
### columns_wide_on=
### columns_wide_off=
### columns_wide_clears_screen
### columns_wide_mode_on_mode_bit-08000000
### parm_mode_val-COLSWIDE-00000000=
### lines=24
##--- MARGINS ---##
### auto_left_margin
auto_left_margin
### eat_newline_glitch
### real_eat_newline_glitch
### auto_wrap_on=
### auto_wrap_off=
### auto_wrap_mode_bit-04000000
### parm_mode_val-AUTOWRAP-00000000=
### set_margins=
##--- CLEAR ---##
### NOTE: write protect mode is supported for clear_screen_unprotected only
### write_protect_on=
### write_protect_off=
### clear_screen_unprotected--=
### clear_screen_unprotected_w_attr--=
clear_screen_unprotected_w_attr--=\EG
### clear_screen--P=
#### no protect off - clear to null
clear_screen=^L
### clr_eos=
### out_clr_eos=
### clr_eos_w_attr=
clr_eos_w_attr=\Ek
### clr_eos_chars=
### set_attr_eos=
### clr_eos_chars_erasable-=
### clear_all=
### clear_all_chars_erasable-=
### clr_eol=
### out_clr_eol=
### clr_eol_w_attr=
clr_eol_w_attr=\EK
### clr_eol_chars=
### set_attr_eol=
### out_clr_eol_chars_erasable=
### clr_bol=
### out_clr_bol=
### clr_bol_w_attr=
### clr_bol_chars=
### set_attr_bol=
### clear_line=
### out_clear_line=
### clear_line_w_attr=
### clear_line_chars=
### set_attr_line=
### erase_chars=
##--- CURSOR ---##
### cursor_address=
cursor_address=\EY%p1%' '%+%c%p2%' '%+%c
### cursor_address_wide=
### cursor_address_wide_starts=number
### row_address=
row_address_addsvp=^K%p1%c
### column_address=
column_address_addsvp=^P%p1%c
### decode_type-0123456789;?=\E[
### cursor_home=
#### NO cursor_home=^A may go to lower left
### cursor_up=
cursor_up=^Z
### cursor_up_at_home_wraps_ll
cursor_up_at_home_wraps_ll
### parm_up_cursor=
### cursor_down=
### parm_down_cursor=
### cursor_right=
#### cursor_right wraps to next line - scroll at bottom right
cursor_right=^F
### parm_right_cursor=
### cursor_left=
#### cursor_left wraps to previous line - to bottom right from home
cursor_left=^U
### parm_left_cursor=
##--- SCROLL ---##
### change_scroll_region=
### scroll_forward=
### scroll_could_be_cursor_only
### parm_index=
### scroll_reverse=
### scroll_reverse_move_col0
### parm_rindex=
### save_cursor=
### restore_cursor=
### auto_scroll_on=
### auto_scroll_off=
### auto_scroll_off_wraps_top
##--- INS/DEL ---##
### insert_line=
insert_line=\EM
### out_insert_line=
### insdel_line_move_col0
insdel_line_move_col0
### insert_line_needs_clear_glitch
### insert_line_sets_attributes
### parm_insert_line=
### out_parm_insert_line=
### delete_line=
delete_line=\El
### parm_delete_line=
### insert_character=
### insert_padding=
### parm_ich=
### enter_insert_mode=
enter_insert_mode=\EF
### exit_insert_mode=
### insert_mode_on_mode_bit-02000000
### parm_mode_val-INSERTON-00000000=
### delete_character=
delete_character=\EE
### parm_delete_character=
##--- ATTRIBUTES URBDOIPA ---##
#### NOTE magic_cookie_glitch=0 for protect and graphics
### attribute-U=
### attribute--U=
### attribute-R=
### attribute-B=
### attribute-D-O=
### attribute-O-D=
### attribute-I=
### attribute-P=
attribute-P=^N
attribute--P=^O
### attribute-A=
attribute-A=\E1
### attribute--A=
attribute--A=\E2
### attribute_next-A-=
##--- MULTIPLE ATTRIBUTES URBDOIPA ---##
##---                              ---##
### 1=dim 2=bold 4=under 5=blink 7=rev 8=invis
### parm_attribute=\E[%7MMm
### parm_attribute--URBDOIPA=\E[;%7MMm
### parm_attribute_out=\E[%s1m
### parm_attribute_out=\E[%s1;%s2m
### parm_attribute_out=\E[%s1;%s2;%s3m
### parm_attribute_out=\E[%s1;%s2;%s3;%s4m
### parm_attribute_out=\E[%s1;%s2;%s3;%s4;%s5m
### parm_attribute_out=\E[%s1;%s2;%s3;%s4;%s5;%s6m
### parm_attribute_out=\E[%s1;%s2;%s3;%s4;%s5;%s6;%s7m
### parm_attribute_out=\E[%s1;%s2;%s3;%s4;%s5;%s6;%s7;%s8m
### parm_attribute_val--URBDOIPA=0
### parm_attribute_val-U-=number
### parm_attribute_val-R-=number
### parm_attribute_val-B-=number
### parm_attribute_val-D-O=number
### parm_attribute_val-O-D=number
### parm_attribute_val-I-=number
### parm_attribute_val-P-=number
### parm_attribute_val-A-=number
### exit_attribute_mode--URBDOIPA=
##--- MAGIC COOKIE ATTRIBUTES ---##
### magic-standout=
### magic-underline=
### magic-reverse=
### magic-blink=
### magic-dim=
### magic-bold=
### magic-secure=
### magic-protected=
### magic-alt_charset=
### nomagic-exit_attribute_mode=
##--- CHARACTER SET ATTRIBUTES URBDOIPA ---##
##---                                   ---##
### vt220 is ( )
### select_character_set_noload
### lock_shift_0=
### single_shift_0=
### select_character_set_0--=
### select_character_set_0--=
### lock_shift_1=
### single_shift_1=
### select_character_set_1--=
### select_character_set_1--=
### lock_shift_2=
### single_shift_2=
### select_character_set_2--=
### select_character_set_2--=
### lock_shift_3=
### single_shift_3=
### select_character_set_3--=
### select_character_set_3--=
##--- LINE ATTRIBUTES ---##
### line_attribute_off=
### line_attribute=
### line_attribute=
##--- CURSOR (cursor_normal cursor_invisible cursor_visible)
### cursor_on=
cursor_on=^X
### cursor_off=
cursor_off=^W
### cursor_on_mode_bit-00800000
### parm_mode_val-CURSORON-00000000=
### cursor_type=
### cursor_type=
### cursor_type_turns_cursor_on
##--- KEYPAD (keypad_xmit keypad_local ) ---##
### enter_appl_keypad_mode=
### exit_appl_keypad_mode=
### appl_keypad_mode_on_mode_bit-00400000
### parm_mode_val-APPKEYON-00000000=
### enter_cursor_key_mode=
### exit_cursor_key_mode=
### cursor_key_mode_on_mode_bit-00200000
### parm_mode_val-CURKEYON-00000000=
##--- TABS ---##
### clear_all_tabs=
### set_tab=
### back_tab=
### NOTE: conflicts with attributes. back_tab=\E0
##--- FUNCTION KEYS ---##
### function_key_clear=
### function_key_clear_mode_yes=number
### function_key_clear_mode_no=number
### parm_pkey_key=
### parm_pkey_key=
### pkey_key=
### out_pkey_key=
### function_key_string_is_hex
### function_key_string_is_ansi
### function_key_string_is_escaped-chars=
### function_key_includes_label
### function_key_len_0_label_only
### function_key-id-delim=
### function_key-id-delim=
### shift_function_key
### function_key-id-delim=
### function_key-id-delim=
##--- CURSOR KEYS ---##
### key_home=
key_home=^A
### key_up=
key_up=^Z
### key_down=
### key_right=
key_right=^F
### key_left=
key_left=^U
### key_backspace=
##--- PRINTER ---##
### transparent_print_on=
transparent_print_on=\E3
### transparent_print_off=
transparent_print_off=\E4
#### ??? TBD print_screen=\Ex
#### ??? transparent print
#### ??? non transparent print on=^R  off=^T
##--- STATUS LINE ---##
### status_off=
status_off=\EB
### status_on=
status_on=\Eb
### status_off_clears_status_type
#### ??? ?? ## TBD status_line_esc_ok
### status_off_clears_status_line
### status_type=
### status_type=
### status_type_turns_status_on
### status_type_ruins_status_line
### status_line=
### status_line_turns_status_on
### status_line_clears_status_line_labels
### status_off_clears_status_line_labels
### status_line_label=\Ez0%8s1\r
### status_line_label=\Ez1%8s1\r
### not_imp-other_labels=\Ez%p1%c%8s1\r
### onstatus=
### out_onstatus=
### onstatus_clear=
##--- MISC ---##
### # ONLY IF PADDING carriage_return=\r$<20/>
### # ONLY IF NOT \n  new_line=
##--- CONTROL_8_BIT ---##
### control_8_bit
### graphics_8_bit
### input_8_bit
### force_CS8
### no_clear_ISTRIP
### terminal_mode_default=number
### terminal_mode_name-CG=
### terminal_mode_select-0-RP=
### out_terminal_mode_switch-0-RP=
### out_terminal_mode_select=
##--- PC_MODE ---##
### pc_mode_on=
### pc_mode_off=
### pc_mode_kd_scancode_driver
### out_pc_mode_on_1=
### out_pc_mode_on_2=
### out_pc_mode_on_3=
### out_pc_mode_off_1=
### out_pc_mode_off_2=
### out_pc_mode_off_3=
### pc_mode_se_on_switch_only
### pc_mode_does_clear_screen
### pc_mode_on_turns_auto_wrap_off
### pc_mode_off_turns_auto_wrap_on
### pc_mode_on_switch_page_number_0
### pc_mode_off_switch_page_number_0
### pc_auto_wrap_on=
### pc_auto_wrap_off=
### pc_cursor_on=
### pc_cursor_off=
### pc_cursor_type=
### pc_cursor_type=
##--- PASTE ---##
### paste_eol-type=
##--- GRAPH_MODE ---##
### graph_mode_start=
### graph_mode_clear=
### out_graph_mode_clear=
##--- MULTIPAGE ---##
### pages=number
### display_page_number=
### parm_display_page_next=
### parm_display_page_prev=
### display_page_next=
### display_page_prev=
##--- SPLIT SCREEN ---##
### NOTE: split_divider must be 77 char starting with blank
### split_divider= qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq
split_divider= ````````````````````````````````````````````````````````````````````````````
### NOTE: split_divider_end must reverse split_divider_start to default
### split_divider_start=
split_divider_start=\E1
### split_divider_end=
split_divider_end=\E2
### pc_split_divider= \315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315
### pc_split_divider_start=
### pc_split_divider_end=
##--- INIT ---##
### enter_ca_mode=
### enter_ca_mode_pc=
### enter_ca_mode_pc_2=
### enter_ca_mode_2=
### exit_ca_mode=
### exit_ca_mode_pc=
### exit_ca_mode_pc_2=
### exit_ca_mode_2=
##--- PARM_MODE ---##
### parm_mode_on=\E[%10M?h
### parm_mode_off=\E[%10M?l
### parm_mode_private_propogates
### mode-bits_on.-bits_off=
### mode-00000000-00000001=\E[?18l
### mode-00000001-00000000=\E[?18h
### parm_mode_val-h.bitson-l.bitson=
### parm_mode_val-00000001-00000000=?18
### multi_mode-bitmask.-bitvalue=
### multi_mode-0000000C-00000000=\E`6$<20>
### multi_mode-0000000C-00000004=\E`A$<20>
### multi_mode-0000000C-00000008=\E`7$<20>
### ignore_mode_bit-80000000
### parm_mode_val-80000000-IGNORE..=?10
### pass_mode_bit-40000000
### parm_mode_val-40000000-PASS....=?11
### notimp_mode_bit-20000000
### parm_mode_val-20000000-NOTIMP..=?12
### ignore=
### ignore_parm=
### pass=
##--- MODE ---##
## protect_char reverse=\E0P dim=\E0A normal=\E0@
multi_mode-0000003C-00000000=\E0P
multi_mode-0000003C-00000004=\E0A
multi_mode-0000003C-00000008=\E0@
multi_mode-0000003C-0000000C=\E0Q
multi_mode-0000003C-00000010=\E0`
### keyboard unlock=\E6 lock=\E5
ignore-keyboard_lock=\E5
pass-keyboard_unlock=\E6
### mode conversational=\EV     form=\ER, etc.
pass=\EV
use=vwpt.fm
