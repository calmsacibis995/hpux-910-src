# FacetTerm  basic4  description file 09/04/91
### Copyright (c) Structured Software Solutions, Inc. 1991. All Rights Reserved.
### @(#) $Header: basic4.FS,v 70.1 92/03/09 16:52:42 ssa Exp $
### xon_xoff
xon_xoff
#
#    !!! IMPORTANT NOTE !!!
#    Set Terminal to: Xonoff 1  ( Setup - line 3 - item 5 )
#
### graphics_escape_control
graphics_escape_control
### graphics_escape_delete
graphics_escape_delete
### columns=80
columns=80
### columns_wide=132
### columns_wide_mode_on_default
### columns_wide_on=
### columns_wide_off=
### columns_wide_clears_screen
### columns_wide_mode_on_mode_bit-08000000
### parm_mode_val-COLSWIDE-00000000=
### lines=24
lines=24
##--- MARGINS ---##
### auto_left_margin
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
write_protect_on=\E&
### write_protect_off=
write_protect_off=\E'
### clear_screen_unprotected--=
clear_screen_unprotected--=\E+
clear_screen_unprotected--=\E;
### clear_screen_unprotected_w_attr--=
### clear_screen--P=
clear_screen--URBDIPA=\E:
clear_screen--URBDIPA=\E*
### clr_eos=
clr_eos_unprotected=\Ey
clr_eos_unprotected=\EY
clr_eos_split_screen_with_refresh
### out_clr_eos=
out_clr_eos=\EY\Eg0
### clr_eos_w_attr=
### clr_eos_chars=
### set_attr_eos=
### clr_eos_chars_erasable-=
### clr_bos=
### clr_bos_chars_erasable-=
### clear_all=
### clear_all_chars_erasable-=
### clr_eol=
clr_fld_unprotected=\Et
clr_fld_unprotected=\ET
### out_clr_eol=
out_clr_fld_unprotected=\ET\Eg0
### clr_eol_w_attr=
### clr_eol_chars=
### set_attr_eol=
### out_clr_eol_chars_erasable=
### clr_bol=
### out_clr_bol=
### clr_bol_w_attr=
### clr_bol_chars=
### set_attr_bol=
### out_clr_bol_chars_erasable=
### clear_line=
### out_clear_line=
### clear_line_w_attr=
### clear_line_chars=
### set_attr_line=
### erase_chars=
##--- CURSOR ---##
### cursor_address=
cursor_address=\E=%p1%' '%+%c%p2%' '%+%c
### cursor_address_wide=
### cursor_address_wide_starts=number
### row_address=
### row_address_addsvp=
### column_address=
### column_address_addsvp=
### decode_type-0123456789;?=\E[
### cursor_home=
cursor_home=^^
cursor_home=\E[H
### cursor_up=
cursor_up=^K
cursor_up=\E[A
### cursor_up_at_home_wraps_ll
### parm_up_cursor=
### cursor_down=
cursor_down=\E[B
### parm_down_cursor=
### cursor_right=
cursor_right=^L
cursor_right=\E[C
### parm_right_cursor=
### cursor_left=
cursor_left=\E[D
### parm_left_cursor=
### cursor_to_ll=
##--- SCROLL ---##
### change_scroll_region=
change_scroll_region=\Es%p1%' '%+%c%p2%' '%+%c
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
### memory_lock=
### memory_unlock=
##--- INS/DEL ---##
### insert_line=
insert_line=\EE
### out_insert_line=
out_insert_line=\EE\Eg0
### insdel_line_move_col0
### insert_line_needs_clear_glitch
### insert_line_sets_attributes
### parm_insert_line=
### out_parm_insert_line=
### delete_line=
delete_line=\ER
### parm_delete_line=
### insert_character=
insert_character=\EQ
### insert_padding=
### parm_ich=
### enter_insert_mode=
enter_insert_mode=\Eq
### exit_insert_mode=
exit_insert_mode=\Er
### insert_mode_on_mode_bit-02000000
### parm_mode_val-INSERTON-00000000=
### delete_character=
delete_character=\EW
### parm_delete_character=
##--- ATTRIBUTES URBDOIPA ---##
attribute-U-RBD=\Eg1
attribute-B-URD=\Eg2
attribute-UB-RD=\Eg3
attribute-R-UBD=\Eg4
attribute-UR-BD=\Eg5
attribute-RB-UD=\Eg6
attribute-URB-D=\Eg7
attribute-D-URB=\Eg8
attribute-UD-RB=\Eg9
attribute-BD-UR=\EgA
attribute-UBD-R=\EgB
attribute-RD-UB=\EgC
attribute-URD-B=\EgD
attribute-RBD-U=\EgE
attribute-URBD-=\EgF
### attribute--U=
### attribute-R=
### attribute-B=
### attribute-D-O=
### attribute-O-D=
### attribute-I=
### attribute-P=
##### attribute-P-=\Eb
##### attribute--P=\Er
attribute-P=\E)
attribute--P=\E(
### attribute-U=
### attribute-A=
attribute-A=\E<
### attribute--A=
attribute--A=\E>
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
exit_attribute_mode--URBDOIP=\Ef
exit_attribute_mode--URBDOIP=\Eg0
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
line_attribute_off=\EF1
### line_attribute=
line_attribute-2H2Wtop=\EF7
line_attribute-2H2Wbot=\EFF
line_attribute-1H2W...=\EF3
not_imp-.......2H1Wtop=\EF5
not_imp-.......2H1Wbot=\EFD
not_imp-.......2H1Wall=\Em
### line_attribute=
##--- CURSOR (cursor_normal cursor_invisible cursor_visible)
### cursor_on=
### cursor_off=
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
### keypad_xmit=
### keypad_local=
### keypad_xmit_mode_bit-00100000
### parm_mode_val-KEYPADXM-00000000=
##--- TABS ---##
### clear_tab=
clear_tab=\E2
### clear_all_tabs=
clear_all_tabs=\E0
### set_tab=
set_tab=\E1
pass-set_tabs_every_8_columns=\E3
### back_tab=
back_tab=\EI
not_imp-tab=\Ei
##--- FUNCTION KEYS ---##
### function_key_type_default=
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
### key_up=
key_up=^K
### key_down=
### key_right=
key_right=^L
### key_left=
### key_backspace=
##--- PRINTER ---##
### transparent_print_on=
transparent_print_on=\EA
### transparent_print_off=
transparent_print_off=\EB
### # TBD print_screen=
### out_transparent_print=
### hp_transparent_print_reply
pass_current-print_page_home_to_cursor..=\EP
pass_current-print_page_home_thru_cursor=\E@
pass_current-print_page_unformatted.....=\E_
pass_current-print_line.................=\Ea
##--- STATUS LINE ---##
### status_off=
### status_on=
### status_off_clears_status_type
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
new_line=^_
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
### NOTE: split_divider_end must reverse split_divider_start to default
### split_divider_start=
### split_divider_end=
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
pass-smooth_scroll_enable=\E8
pass-smooth_scroll_disable=\E9
not_imp-read_cursor_position=\E?
not_imp-read_cursor_content=\E\^
pass-unlock_keyboard=\E"
not_imp-lock_keyboard=\E#
pass_current-send_terminal_id=\Ed
not_imp-set_attr_all=\EH%p1%c
not_imp-set_attr_eol_and_leave_write_protect_space=\EG%p1%c
not_imp-enable_special_functions=\E$
not_imp-disable_special_functions=\E%%
not_imp-send_page_unprotected_home_thru_cursor.=\E5
not_imp-send_page_home_thru_cursor.............=\E7
not_imp-send_line_unprotected_first_thru_cursor=\E4
not_imp-send_line_first_thru_cursor............=\E6
not_imp-partial_send_FS_TO_FS..................=\ES
## master reset
substitute_new=\E*\E(\Eg0
substitute_for=\EZ
not_imp-enter_mode_enable=\EV
not_imp-enter_mode_disable=\Ev
perwindow-12-CSQL--------=\E]%10f1
perwindow_default-.......=\E]\s\s\s\s\s\s\s\s\s\s
not_imp-read_title=\En
not_imp-program_func_keys=\Ek
not_imp-show_func_keys=\Ej
not_imp-envoke_func_key=\E\\%p1%02d
pass-clock_on..............=\Eh0
pass-clock_off.............=\Eh1
pass-clock_advance_hour....=\Eh2
pass-clock_advance_minute..=\Eh3
not_imp-clock_read_all.....=\Eh4
pass-clock_set_hour........=\Eh5%p1%c
pass-clock_set_min.........=\Eh6%p1%c
pass-clock_set_second......=\Eh7%p1%c
not_imp-enter_setup=\Ex
## ###############################################################
## FacetTerm Menu Capabilities for a WYSE 60 06/04/91
## Copyright (c) Structured Software Solutions, Inc. 1991.  All Rights Reserved.
## @(#) $Header: basic4.FS,v 70.1 92/03/09 16:52:42 ssa Exp $
## ##-menu-cap-init_string=
##-menu-cap-cursor_address=\E=%p1%' '%+%c%p2%' '%+%c
##-menu-cap-clear_screen=\E*
##-menu-cap-clr_eol=\ET
## ##-menu-cap-ena_acs=
## ##-menu-cap-enter_alt_charset_mode=
## ##-menu-cap-exit_alt_charset_mode=
## ##-menu-cap-acs_chars=
## ##-menu-cap-keypad_xmit=
##-menu-cap-no_attr=\Eg0
##-menu-cap-shadow_attr=\Eg0
##-menu-cap-item_attr=\Eg0
##-menu-cap-box_attr=\EgC
##-menu-cap-title_attr=\EgC
##-menu-cap-highlight_attr=\Eg4
##-menu-cap-highlight_blink_attr=\Eg6
## ##-menu-cap-magic_cookie=0
## ##-menu-cap-ceol_standout_glitch
##-menu-cap-key_up=^K
## ##-menu-cap-key_down=
##-menu-cap-key_right=^L
## ##-menu-cap-key_left=
## ##-menu-cap-key_ignore=
## ###############################################################
