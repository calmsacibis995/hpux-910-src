# FacetTerm  list_of_all_attributes  description file 09/16/91 i
## ## NOTE: Lines which have # in column 1 are comments.
## ##       Comment lines with which begin with # and space are echoed to
## ##       the terminal as the terminal descripition is read.
## Copyright (c) Structured Software Solutions, Inc. 1991.  All Rights Reserved.
## @(#) $Header: ALL_ATTRS.FS,v 70.1 92/03/09 16:12:04 ssa Exp $
## xon_xoff
### TI_TO_FI xon_xoff
## allow_tabs
## graphics_escape_control
## graphics_escape_delete
## delete_is_a_character
## tilde_glitch
## columns=80
## use_COLS_if_set
### TI_TO_FI columns=
## columns_wide=132
## columns_wide_mode_on_default
## columns_wide_on=
## columns_wide_off=
## columns_wide_clears_screen
## columns_wide_mode_on_mode_bit-08000000
## columns_wide_clears_onstatus
## columns_wide_switch_resets_scroll_region
## columns_wide_switch_reload_scroll_region
## parm_mode_val-COLSWIDE-00000000=
## lines=24
### TI_TO_FI lines=
## use_LINES_if_set
## use_TIOCWINSZ_if_set
## rows_change=
## rows_change_se_on_switch_only
## rows_change_attributes_off
## rows_change_clears_pages
## rows_change_clears_perwindow
## rows_change_does_clear_screen
## rows_change_resets_character_set
## rows_change_resets_insert_mode
## rows_change_resets_save_cursor
## rows_change_resets_scroll_region
## rows_change_switch_page_number_0
## extra_data_row_off=
## extra_data_row_on=
## extra_data_row_resets_scroll_region
## out_extra_data_row_off=
## out_extra_data_row_on=
## out_rows_change_extra_data_row_off=
## out_rows_change_extra_data_row_on=
## ##--- MARGINS ---##
## auto_left_margin
### TI_TO_FI auto_left_margin
### TI_TO_FI ASSUMED auto_right_margin
## eat_newline_glitch
### TI_TO_FI eat_newline_glitch
## real_eat_newline_glitch
## auto_wrap_on=
### TI_TO_FI RENAME enter_am_mode= auto_wrap_on=
## auto_wrap_off=
### TI_TO_FI RENAME exit_am_mode= auto_wrap_off=
## auto_wrap_mode_bit-04000000
## parm_mode_val-AUTOWRAP-00000000=
## set_margins=
## ##--- CLEAR ---##
## ### NOTE: write protect mode is supported for clear_screen_unprotected only
## write_protect_on=
## write_protect_off=
## clear_screen_unprotected--=
## clear_screen_unprotected_w_attr--=
## clear_screen--P=
### TI_TO_FI clear_screen=
## clr_eos=
### TI_TO_FI clr_eos=
## out_clr_eos=
## clr_eos_w_attr=
## clr_eos_split_screen_with_refresh
## clr_eos_unprotected=
## clr_eos_unprotected_w_attr=
## clr_eos_chars=
## set_attr_eos=
## clr_eos_chars_erasable-=
## clr_bos=
## clr_bos_chars_erasable-=
## clear_all=
## clear_all_w_attr--P=
## clear_all_chars_erasable-=
## clr_eol=
### TI_TO_FI clr_eol=
## out_clr_eol=
## clr_eol_w_attr=
## clr_fld_unprotected=
## clr_fld_unprotected_w_attr=
## clr_eol_chars=
## set_attr_eol=
## out_clr_eol_chars_erasable=
## clr_bol=
### TI_TO_FI clr_bol=
## out_clr_bol=
## clr_bol_w_attr=
## clr_bol_chars=
## set_attr_bol=
## out_clr_bol_chars_erasable=
## clear_line=
## out_clear_line=
## clear_line_w_attr=
## clear_line_chars=
## set_attr_line=
## erase_chars=
### TI_TO_FI erase_chars=
## ##--- CURSOR ---##
## cursor_address=
### TI_TO_FI cursor_address=
## cursor_address_wide=
## cursor_address_wide_starts=number
## row_address=
### TI_TO_FI row_address=
## row_address_addsvp=
## row_address_exit_attribute_set_attr_line=
## column_address=
### TI_TO_FI column_address=
## column_address_addsvp=
## column_address_parm_down_cursor=
## column_address_parm_up_cursor=
## decode_type-0123456789;?=\E[
## cursor_home=
### TI_TO_FI cursor_home=
## cursor_home_down=
## cursor_up=
### TI_TO_FI cursor_up=
## cursor_up_at_home_wraps_ll
## parm_up_cursor=
### TI_TO_FI parm_up_cursor=
## cursor_down=
### TI_TO_FI NOTE cursor_down=\n
### TI_TO_FI NOTE cursor_down=^J
### TI_TO_FI cursor_down=
## parm_down_cursor=
### TI_TO_FI parm_down_cursor=
## cursor_right=
### TI_TO_FI cursor_right=
## parm_right_cursor=
### TI_TO_FI parm_right_cursor=
## cursor_left=
### TI_TO_FI NOTE cursor_left=\b
### TI_TO_FI cursor_left=
## parm_left_cursor=
### TI_TO_FI parm_left_cursor=
## cursor_to_ll=
### TI_TO_FI cursor_to_ll=
## ##--- SCROLL ---##
## change_scroll_region=
### TI_TO_FI change_scroll_region=
## scroll_forward=
### TI_TO_FI NOTE scroll_forward=\n
### TI_TO_FI NOTE scroll_forward=^J
### TI_TO_FI scroll_forward=
## scroll_could_be_cursor_only
## parm_index=
### TI_TO_FI parm_index=
## scroll_reverse=
### TI_TO_FI scroll_reverse=
## scroll_reverse_move_col0
## parm_rindex=
### TI_TO_FI parm_rindex=
## save_cursor=
### TI_TO_FI save_cursor=
## restore_cursor=
### TI_TO_FI restore_cursor=
## auto_scroll_on=
## auto_scroll_off=
## auto_scroll_off_wraps_top
## memory_lock=
### TI_TO_FI memory_lock=
## memory_unlock=
### TI_TO_FI memory_unlock=
## ##--- INS/DEL ---##
## insert_line=
### TI_TO_FI insert_line=
## out_insert_line=
## insdel_line_move_col0
## insert_line_needs_clear_glitch
## insert_line_sets_attributes
## parm_insert_line=
### TI_TO_FI parm_insert_line=
## out_parm_insert_line=
## delete_line=
### TI_TO_FI delete_line=
## parm_delete_line=
### TI_TO_FI parm_delete_line=
## insert_character=
### TI_TO_FI insert_character=
## insert_padding=
### TI_TO_FI insert_padding=
## parm_ich=
### TI_TO_FI parm_ich=
## enter_insert_mode=
### TI_TO_FI enter_insert_mode=
## exit_insert_mode=
### TI_TO_FI exit_insert_mode=
## insert_mode_on_mode_bit-02000000
## parm_mode_val-INSERTON-00000000=
## delete_character=
### TI_TO_FI delete_character=
## parm_delete_character=
### TI_TO_FI RENAME parm_dch= parm_delete_character=
## ##--- ATTRIBUTES URBDOIPA ---##
## attribute-U=
## attribute--U=
## attribute-R=
## attribute-B=
## attribute-D-O=
## attribute-O-D=
## attribute-I=
## attribute-P=
## attribute-A=
## attribute--A=
## attribute_next-A-=
## ##--- MULTIPLE ATTRIBUTES URBDOIPA ---##
## ##---                              ---##
### TI_TO_FI NOTE set_attributes=
### 1=dim 2=bold 4=under 5=blink 7=rev 8=invis
## parm_attribute=\E[%7MMm
## parm_attribute--URBDOIPA=\E[;%7MMm
## parm_attribute_out=\E[%s1m
## parm_attribute_out=\E[%s1;%s2m
## parm_attribute_out=\E[%s1;%s2;%s3m
## parm_attribute_out=\E[%s1;%s2;%s3;%s4m
## parm_attribute_out=\E[%s1;%s2;%s3;%s4;%s5m
## parm_attribute_out=\E[%s1;%s2;%s3;%s4;%s5;%s6m
## parm_attribute_out=\E[%s1;%s2;%s3;%s4;%s5;%s6;%s7m
## parm_attribute_out=\E[%s1;%s2;%s3;%s4;%s5;%s6;%s7;%s8m
## parm_attribute_val--URBDOIPA=0
## parm_attribute_val-U-=number
### TI_TO_FI NOTE enter_underline_mode=
### TI_TO_FI NOTE exit_underline_mode=
## parm_attribute_val-R-=number
### TI_TO_FI NOTE enter_reverse_mode=
### TI_TO_FI NOTE enter_standout_mode=
### TI_TO_FI NOTE exit_standout_mode=
## parm_attribute_val-B-=number
### TI_TO_FI NOTE enter_blink_mode=
## parm_attribute_val-D-O=number
## parm_attribute_val-O-D=number
### TI_TO_FI NOTE enter_bold_mode=
## parm_attribute_val-I-=number
## parm_attribute_val-P-=number
## parm_attribute_val-A-=number
## exit_attribute_mode--URBDOIPA=
### TI_TO_FI NOTE exit_attribute_mode=
## ##--- MAGIC COOKIE ATTRIBUTES ---##
## magic-standout=
## magic-underline=
## magic-reverse=
## magic-blink=
## magic-dim=
## magic-bold=
## magic-secure=
## magic-protected=
## magic-alt_charset=
## nomagic-exit_attribute_mode=
## out_magic=
## ##--- CHARACTER SET ATTRIBUTES URBDOIPA ---##
## ##---                                   ---##
### vt220 is ( )
## select_character_set_noload
## select_character_set_sets_current
## lock_shift_0=
### TI_TO_FI NOTE exit_alt_charset_mode=
## single_shift_0=
## select_character_set_0--=
## select_character_set_0--=
## lock_shift_1=
### TI_TO_FI NOTE enter_alt_charset_mode=
## single_shift_1=
## select_character_set_1--=
## select_character_set_1--=
## lock_shift_2=
## single_shift_2=
## select_character_set_2--=
## select_character_set_2--=
## lock_shift_3=
## single_shift_3=
## select_character_set_3--=
## select_character_set_3--=
### TI_TO_FI NOTE ena_acs=
### TI_TO_FI NOTE acs_chars=
## ##--- LINE ATTRIBUTES ---##
## line_attribute_off=
## line_attribute=
## line_attribute=
## ##--- OTHER ATTRIBUTES ---##
## hp_attribute=
## hp_color=
## hp_charset_norm=
## hp_charset_alt=
## hp_charset_select=
## ibm_attribute=
## ##--- CURSOR (cursor_normal cursor_invisible cursor_visible)
## cursor_on=
## cursor_off=
## cursor_on_mode_bit-00800000
## parm_mode_val-CURSORON-00000000=
## cursor_type=
## cursor_type=
## cursor_type_turns_cursor_on
## ##--- KEYPAD (keypad_xmit keypad_local ) ---##
## enter_appl_keypad_mode=
## exit_appl_keypad_mode=
## appl_keypad_mode_on_mode_bit-00400000
## parm_mode_val-APPKEYON-00000000=
## enter_cursor_key_mode=
## exit_cursor_key_mode=
## cursor_key_mode_on_mode_bit-00200000
## parm_mode_val-CURKEYON-00000000=
## keypad_xmit=
### TI_TO_FI keypad_xmit=
## keypad_local=
### TI_TO_FI keypad_local=
## keypad_xmit_mode_bit-00100000
## parm_mode_val-KEYPADXM-00000000=
## ##--- TABS ---##
## clear_tab=
## clear_all_tabs=
### TI_TO_FI clear_all_tabs=
## set_tab=
### TI_TO_FI set_tab=
## back_tab=
### TI_TO_FI back_tab=
### TI_TO_FI ASSUMED tab=\t
### TI_TO_FI ASSUMED tab=^I
### TI_TO_FI ASSUMED init_tabs=8
## ##--- FUNCTION KEYS ---##
## function_key_file=
## function_key_default=
## function_key_type_default=
## function_key_type_set=
## function_key_clear=
## function_key_clear_mode_yes=number
## function_key_clear_mode_no=number
## function_key_clear_mode_default=number
## parm_pkey_key=
## parm_pkey_key=
## pkey_key=
## out_pkey_key=
## pkey_key_clear=
## out_pkey_key_clear=
## function_key_string_is_hex
## function_key_string_is_ansi
## function_key_string_is_escaped-chars=
## function_key_includes_label
## function_key_label_max_len
## function_key_len_0_label_only
## function_key_len_0_func_only
## function_key-id-delim=
## function_key-id-delim=
## shift_function_key
## function_key-id-delim=
## function_key-id-delim=
### TI_TO_FI NOTE key_f0=
### TI_TO_FI NOTE key_f1=
### TI_TO_FI NOTE key_f2=
### TI_TO_FI NOTE key_f3=
### TI_TO_FI NOTE key_f4=
### TI_TO_FI NOTE key_f5=
### TI_TO_FI NOTE key_f6=
### TI_TO_FI NOTE key_f7=
### TI_TO_FI NOTE key_f8=
### TI_TO_FI NOTE key_f9=
### TI_TO_FI NOTE key_f10=
### TI_TO_FI NOTE key_f11=
### TI_TO_FI NOTE key_f12=
### TI_TO_FI NOTE key_f13=
### TI_TO_FI NOTE key_f14=
### TI_TO_FI NOTE key_f15=
### TI_TO_FI NOTE key_f16=
### TI_TO_FI NOTE key_f17=
### TI_TO_FI NOTE key_f18=
### TI_TO_FI NOTE key_f19=
### TI_TO_FI NOTE key_f20=
### TI_TO_FI NOTE key_f21=
### TI_TO_FI NOTE key_f22=
### TI_TO_FI NOTE key_f23=
### TI_TO_FI NOTE key_f24=
### TI_TO_FI NOTE key_f25=
### TI_TO_FI NOTE key_f26=
### TI_TO_FI NOTE key_f27=
### TI_TO_FI NOTE key_f28=
### TI_TO_FI NOTE key_f29=
### TI_TO_FI NOTE key_f30=
### TI_TO_FI NOTE key_f31=
### TI_TO_FI NOTE key_f32=
### TI_TO_FI NOTE key_f33=
### TI_TO_FI NOTE key_f34=
### TI_TO_FI NOTE key_f35=
### TI_TO_FI NOTE key_f36=
### TI_TO_FI NOTE key_f37=
### TI_TO_FI NOTE key_f38=
### TI_TO_FI NOTE key_f39=
### TI_TO_FI NOTE lab_f0=
### TI_TO_FI NOTE lab_f1=
### TI_TO_FI NOTE lab_f2=
### TI_TO_FI NOTE lab_f3=
### TI_TO_FI NOTE lab_f4=
### TI_TO_FI NOTE lab_f5=
### TI_TO_FI NOTE lab_f6=
### TI_TO_FI NOTE lab_f7=
### TI_TO_FI NOTE lab_f8=
## ##--- CURSOR KEYS ---##
## key_home=
### TI_TO_FI key_home=
## key_up=
### TI_TO_FI key_up=
## key_down=
### TI_TO_FI key_down=
## key_right=
### TI_TO_FI key_right=
## key_left=
### TI_TO_FI key_left=
## key_backspace=
### TI_TO_FI ASSUMED key_backspace=\b
### TI_TO_FI ASSUMED key_backspace=^H
### TI_TO_FI key_backspace=
### TI_TO_FI NOTE key_ic=
### TI_TO_FI NOTE key_dc=
### TI_TO_FI NOTE key_il=
### TI_TO_FI NOTE key_dl=
### TI_TO_FI NOTE key_npage=
### TI_TO_FI NOTE key_ppage=
## ##--- PRINTER ---##
### TI_TO_FI NOTE prtr_silent
## transparent_print_on=
### TI_TO_FI NOTE_RENAME prtr_on= transparent_print_on=
## transparent_print_off=
### TI_TO_FI NOTE_RENAME prtr_off= transparent_print_off=
### TI_TO_FI NOTE print_screen=
## out_transparent_print=
## transparent_print_hold_if_partial
## out_hp_transparent_print=
## hp_transparent_print_reply
## hp_transparent_print_enq_ack
## ##--- STATUS LINE ---##
### TI_TO_FI NOTE has_status_line
## status_off=
## status_on=
## status_off_clears_status_type
## status_off_clears_status_line
## status_type=
## status_type=
## status_type_turns_status_on
## status_type_ruins_status_line
## status_line=
## status_line_turns_status_on
## status_line_clears_status_line_labels
## status_off_clears_status_line_labels
## status_line_label=\Ez0%8s1\r
## status_line_label=\Ez1%8s1\r
## not_imp-other_labels=\Ez%p1%c%8s1\r
## onstatus=
### TI_TO_FI RENAME to_status_line= onstatus=
## out_onstatus=
## onstatus_clear=
### TI_TO_FI NOTE from_status_line=
## ##--- MISC ---##
## # ONLY IF PADDING carriage_return=\r$<20/>
### TI_TO_FI ASSUMED carriage_return=\r
### TI_TO_FI ASSUMED carriage_return=^M
### TI_TO_FI carriage_return=
## # ONLY IF NOT \n  new_line=
### TI_TO_FI new_line=
### TI_TO_FI RENAME newline= new_line=
### TI_TO_FI ASSUMED bell=^G
## draw_box--2-A-:-A-3-A-6-A-5-A-1-A-=
## draw_box_width_height--2-A-:-A-3-A-6-A-5-A-1-A-=
## out_screen_saver_start=
## screen_saver_timer=
## substitute_new=
## substitute_for=
## ##--- CONTROL_8_BIT ---##
## control_8_bit
## graphics_8_bit
## input_8_bit
## force_CS8
## no_clear_ISTRIP
## terminal_mode_default=number
## terminal_mode_name-CG=
## terminal_mode_select-0-RP=
## out_terminal_mode_switch-0-RP=
## out_terminal_mode_select=
## ##--- PC_MODE ---##
## pc_mode_on=
## pc_mode_off=
## pc_mode_kd_scancode_driver
## out_pc_mode_on_1=
## out_pc_mode_on_2=
## out_pc_mode_on_3=
## out_pc_mode_off_1=
## out_pc_mode_off_2=
## out_pc_mode_off_3=
## pc_mode_se_on_switch_only
## pc_mode_does_clear_screen
## pc_mode_on_turns_auto_wrap_off
## pc_mode_off_turns_auto_wrap_on
## pc_mode_on_switch_page_number_0
## pc_mode_off_switch_page_number_0
## pc_mode_defaults_function_keys
## pc_mode_resets_appl_keypad_mode
## pc_mode_resets_attributes
## pc_mode_resets_character_set
## pc_mode_resets_insert_mode
## pc_mode_resets_origin_mode
## pc_mode_resets_save_cursor
## pc_mode_resets_scroll_region
## pc_mode_sets_cursor_on
## pc_mode_sets_mode
## pc_auto_wrap_on=
## pc_auto_wrap_off=
## pc_cursor_on=
## pc_cursor_off=
## pc_cursor_type=
## pc_cursor_type=
## ##--- PASTE ---##
## paste_eol-type=
## ##--- GRAPH_MODE ---##
## graph_mode_start=
## graph_mode_clear=
## out_graph_mode_clear=
## graph_screen_on=
## graph_screen_on_side_effect=
## graph_screen_off=
## graph_screen_off_side_effect=
## out_graph_screen_off=
## ##--- MULTIPAGE ---##
## pages=number
## display_page_number=
## parm_display_page_next=
## parm_display_page_prev=
## display_page_next=
## display_page_prev=
## ##--- SPLIT SCREEN ---##
## ### NOTE: split_divider must be 77 char starting with blank
## split_divider= qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq
## ### NOTE: split_divider_end must reverse split_divider_start to default
## split_divider_start=
## split_divider_end=
## pc_split_divider= \315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315
## pc_split_divider_start=
## pc_split_divider_end=
## ##--- INIT ---##
### TI_TO_FI PASS enter_ca_mode=
## enter_ca_mode=
## enter_ca_mode_pc=
## enter_ca_mode_pc_2=
## enter_ca_mode_2=
### TI_TO_FI PASS exit_ca_mode=
## exit_ca_mode=
## exit_ca_mode_pc=
## exit_ca_mode_pc_2=
## exit_ca_mode_2=
### TI_TO_FI PASS reset_1string=
### TI_TO_FI PASS reset_2string=
### TI_TO_FI PASS reset_3string=
### TI_TO_FI NOTE init_file=
### TI_TO_FI PASS init_1string=
### TI_TO_FI PASS init_2string=
### TI_TO_FI PASS init_3string=
## ##--- SCROLL_MEMORY ---##
## lines_scroll_memory=
## out_clr_memory_below=
## scroll_refresh_end=
## scroll_refresh_start=
## out_scroll_memory_clear=
## meta_cursor_home_down=
## meta_roll_down=
## out_meta_roll_down=
## meta_roll_up=
## out_meta_roll_up=
## in_meta_roll_cancel=
## INSTALL_ALL_PERSONALITIES
## NEW_PERSONALITY
## ##--- TERMINAL_RESPONSE ---##
## answer=
## question=
## question=
## ##--- IBM CONTROLS ---##
## ibm_control=
## ibm_control_default=
## ibm_control_force_mask=
## ibm_control_force_value=
## ibm_control_auto_wrap=
## ibm_control_auto_wrap_off_out=
## ibm_control_auto_wrap_on_out=
## ##--- PERWINDOW ---##
## perwindow-length-CLWROSIQFA-clears-cleared_by-order_group------=
## perwindow_also-................................................=
## perwindow_default-.............................................=
## perwindow_pad=
## perwindow_before=
## perwindow_after=
## perwindow_special=
## perwindow_variable=
## set=
## ##--- PARM_MODE ---##
## parm_mode_on=\E[%10M?h
## parm_mode_off=\E[%10M?l
## parm_mode_private_propogates
## mode-bits_on.-bits_off=
## mode-00000000-00000001=\E[?18l
## mode-00000001-00000000=\E[?18h
## parm_mode_val-h.bitson-l.bitson=
## parm_mode_val-00000001-00000000=?18
## multi_mode-bitmask.-bitvalue=
## multi_mode-0000000C-00000000=\E`6$<20>
## multi_mode-0000000C-00000004=\E`A$<20>
## multi_mode-0000000C-00000008=\E`7$<20>
## ignore_mode_bit-80000000
## parm_mode_val-80000000-IGNORE..=?10
## pass_mode_bit-40000000
## parm_mode_val-40000000-PASS....=?11
## notimp_mode_bit-20000000
## parm_mode_val-20000000-NOTIMP..=?12
## ignore=
## ignore_parm=
## pass=
## pass_current=
### TI_TO_FI NOTE flash_screen=
## ##--- MODE ---##
## ## #########################################################################
## ## FacetTerm Menu Capabilities for XXXX 99/99/99
## ## Copyright (c) Structured Software Solutions, Inc. 1991.  All Rights Reserved.
## ## @(#) $Header: ALL_ATTRS.FS,v 70.1 92/03/09 16:12:04 ssa Exp $
## ##-menu-cap-init_string=
## ##-menu-cap-cursor_address=
## ##-menu-cap-clear_screen=
## ##-menu-cap-clr_eol=
## ##-menu-cap-ena_acs=
## ##-menu-cap-enter_alt_charset_mode=
## ##-menu-cap-exit_alt_charset_mode=
## ## j BR k UR l UL m BL q H  t LT u RT v BT w UT x V  n MT
## ##-menu-cap-acs_chars=j.k.l.m.q.t.u.v.w.x.n.
## ##-menu-cap-keypad_xmit=
## ##-menu-cap-no_attr=
## ##-menu-cap-shadow_attr=
## ##-menu-cap-item_attr=
## ##-menu-cap-box_attr=
## ##-menu-cap-title_attr=
## ##-menu-cap-highlight_attr=
## ##-menu-cap-highlight_blink_attr=
## ##-menu-cap-magic_cookie=0
## ##-menu-cap-ceol_standout_glitch
## ##-menu-cap-key_home=
## ##-menu-cap-key_up=
## ##-menu-cap-key_down=
## ##-menu-cap-key_right=
## ##-menu-cap-key_left=
## ##-menu-cap-key_ignore=
## ## #########################################################################
