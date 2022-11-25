# FacetTerm  f1  (80/132 column only)  description file 09/19/91
#      Freedom 1 terminal
## Copyright (c) Structured Software Solutions, Inc. 1991.  All Rights Reserved.
## @(#) $Header: f1.FS,v 70.1 92/03/09 16:52:29 ssa Exp $
### xon_xoff
xon_xoff
### graphics_escape_control
##### ##CSI next line commented
##### *** graphics_escape_control
### graphics_escape_delete
### columns=80
columns=80
### columns_wide=132
columns_wide=132
### columns_wide_mode_on_default
### columns_wide_on=
columns_wide_on=\E^V1
### columns_wide_off=
columns_wide_off=\E^V0
### columns_wide_clears_screen
### columns_wide_mode_on_mode_bit-08000000
### columns_wide_clears_onstatus
### columns_wide_switch_resets_scroll_region
### columns_wide_switch_reload_scroll_region
### parm_mode_val-COLSWIDE-00000000=
### lines=24
lines=24
##--- MARGINS ---##
### auto_left_margin
*** auto_left_margin
### eat_newline_glitch
### real_eat_newline_glitch
##### ##CSI next line commented
##### ##real_eat_newline_glitch
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
### clear_screen_unprotected_w_attr--=
### clear_screen--P=
##### clear unprotected to write protected insert character
clear_screen--=\E,
##### clear unprotected to insert character
clear_screen--=^Z
##### clear unprotected to insert character
clear_screen--=\E;
##### clear unprotected to insert character
clear_screen--=\E+
##### clear unprotected to null
clear_screen--=\E:
##### clear all to null
clear_screen--P=\E*
### clr_eos=
##### erase to end of page with null
clr_eos=\Ey
##### erase to end of page
clr_eos=\EY
### out_clr_eos=
### clr_eos_w_attr=
### clr_eos_chars=
### set_attr_eos=
### clr_eos_chars_erasable-=
### clr_bos=
### clr_bos_chars_erasable-=
### clear_all=
### clear_all_chars_erasable-=
### clr_eol=
##### erase to end of line with null
clr_eol=\Et
##### erase to end of line
clr_eol=\ET
### out_clr_eol=
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
cursor_address=\E-0%p1%' '%+%c%p2%' '%+%c
cursor_address=\E=%p1%' '%+%c%p2%' '%+%c
not_imp-cursor_address_second_page=\E-1%p1%' '%+%c%p2%' '%+%c
### cursor_address_wide=
cursor_address_wide=\E^]%i  %p1%dR  %p2%dC
cursor_address_wide=\E^]%i  %p1%dR %p2%dC
cursor_address_wide=\E^]%i  %p1%dR%p2%dC
cursor_address_wide=\E^]%i %p1%dR  %p2%dC
cursor_address_wide=\E^]%i %p1%dR %p2%dC
cursor_address_wide=\E^]%i %p1%dR%p2%dC
cursor_address_wide=\E^]%i%p1%dR  %p2%dC
cursor_address_wide=\E^]%i%p1%dR %p2%dC
cursor_address_wide=\E^]%i%p1%dR%p2%dC
### cursor_address_wide_starts=number
cursor_address_wide_starts=80
### row_address=
row_address=\E[%p1%' '%+%c
### row_address_addsvp=
### column_address=
column_address=\E]%p1%' '%+%c
### column_address_addsvp=
### decode_type-0123456789;?=\E[
### cursor_home=
cursor_home=^^
### cursor_up=
cursor_up=^K
### cursor_up_at_home_wraps_ll
### parm_up_cursor=
### cursor_down=
cursor_down=^V
### parm_down_cursor=
### cursor_right=
cursor_right=^L
### parm_right_cursor=
### cursor_left=
### parm_left_cursor=
### cursor_to_ll=
##--- SCROLL ---##
### change_scroll_region=
change_scroll_region=\Em0%p1%' '%+%c%p2%' '%+%c
### scroll_forward=
### scroll_could_be_cursor_only
scroll_could_be_cursor_only
### parm_index=
### scroll_reverse=
##### reverse linefeed
scroll_reverse=\Ej
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
### insdel_line_move_col0
insdel_line_move_col0
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
##CSI next block copied from wy60com.fc and then modified
##--- ATTRIBUTES URBDOIPA ---##
### attribute-U=
### attribute--U=
### attribute-R=
### attribute-B=
### attribute-D-O=
### attribute-O-D=
### attribute-I=
### attribute-P=
### attribute-A=
### attribute--A=
attribute-I-URBD=\EG1
attribute-B-URDI=\EG2
attribute-R-UBDI=\EG4
attribute-RB-UDI=\EG6
attribute-U-RBDI=\EG8
attribute-UB-RDI=\EG:
attribute-UR-BDI=\EG<
attribute-URB-DI=\EG>
attribute-D-URBI=\EG@
attribute-BD-URI=\EGB
attribute-RD-UBI=\EGD
attribute-RBD-UI=\EGF
attribute-UD-RBI=\EGH
attribute-UBD-RI=\EGJ
attribute-URD-BI=\EGL
attribute-URBD-I=\EGN
attribute-P=\E)
attribute--P=\E(
attribute-A=\E$
attribute--A=\E%
### attribute_next-A-=
attribute_next-A-=\E^G
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
exit_attribute_mode--URBDI=\EG0
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
not_imp-select_line_attribute=\E^L%p1%c
### line_attribute=
##--- CURSOR (cursor_normal cursor_invisible cursor_visible)
### cursor_on=
cursor_on=\E.1
### cursor_off=
cursor_off=\E.0
### cursor_on_mode_bit-00800000
### parm_mode_val-CURSORON-00000000=
### cursor_type=
cursor_type-block_steady=\E.2
cursor_type-under_blink=\E.3
cursor_type-under_steady=\E.4
cursor_type-block_blink=\E.5
### cursor_type=
### cursor_type_turns_cursor_on
cursor_type_turns_cursor_on
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
clear_all_tabs=\E3
### set_tab=
set_tab=\E1
### back_tab=
back_tab=\EI
##--- FUNCTION KEYS ---##
### function_key_default=
function_key_default=\E|^?^?
### function_key_type_default=
function_key_type_default=5
### function_key_type_set=
### function_key_clear=
### function_key_clear_mode_yes=number
### function_key_clear_mode_no=number
### parm_pkey_key=
### parm_pkey_key=
### pkey_key=
pkey_key=\E|%p1%c%1fB%s1^Y
### out_pkey_key=
### function_key_string_is_hex
### function_key_string_is_ansi
### function_key_string_is_escaped-chars=
### function_key_includes_label
### function_key_len_0_label_only
### function_key-id-delim=
function_key-1-=^A@\r
function_key-2-=^AA\r
function_key-3-=^AB\r
function_key-4-=^AC\r
function_key-5-=^AD\r
function_key-6-=^AE\r
function_key-7-=^AF\r
function_key-8-=^AG\r
function_key-9-=^AH\r
function_key-:-=^AI\r
function_key-^-=^AJ\r
function_key-`-=^AK\r
function_key-b-=^AL\r
function_key-d-=^AM\r
function_key-f-=^AN\r
function_key-h-=^AO\r
### function_key-id-delim=
### shift_function_key
shift_function_key
### function_key-id-delim=
function_key-<-=^A`\r
function_key-61-=^Aa\r
function_key->-=^Ab\r
function_key-?-=^Ac\r
function_key-@-=^Ad\r
function_key-A-=^Ae\r
function_key-B-=^Af\r
function_key-C-=^Ag\r
function_key-D-=^Ah\r
function_key-E-=^Ai\r
function_key-_-=^Aj\r
function_key-a-=^Ak\r
function_key-c-=^Al\r
function_key-e-=^Am\r
function_key-g-=^An\r
function_key-i-=^Ao\r
### function_key-id-delim=
## edit keys
## down arrow ^O
function_key-15-=^V
## backspace
function_key-Z-=^H
##--- CURSOR KEYS ---##
### key_home=
key_home=^^
### key_up=
key_up=^K
### key_down=
key_down=^V
### key_right=
key_right=^L
### key_left=
### key_backspace=
##--- PRINTER ---##
### transparent_print_on=
transparent_print_on=\E`
### transparent_print_off=
transparent_print_off=\Ea
### # TBD print_screen=
### out_transparent_print=
### hp_transparent_print_reply
pass-....copy_print_disable.=\EA
not_imp-.copy_print_enable..=\E@
pass-....bidirectional_port_disable.=^T
not_imp-.bidirectional_port_enable..=^R
##--- STATUS LINE ---##
### status_off=
status_off-display_status_line=\Eh
### status_on=
status_on-display_user_line=\Eg
### status_off_clears_status_type
### status_off_clears_status_line
### status_type=
### status_type=
### status_type_turns_status_on
### status_type_ruins_status_line
### status_line=
status_line=\Ef%80s1\r
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
graphics_8_bit
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
##### COLUMNS_WIDE_CLEARS_PAGES pages=2
### display_page_number=
### parm_display_page_next=
### parm_display_page_prev=
### display_page_next=
##### COLUMNS_WIDE_CLEARS_PAGES display_page_next=\EK
### display_page_prev=
##### COLUMNS_WIDE_CLEARS_PAGES display_page_prev=\EJ
##--- SPLIT SCREEN ---##
### NOTE: split_divider must be 77 char starting with blank
### split_divider= qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq
### NOTE: split_divider_end must reverse split_divider_start to default
### split_divider_start=
### split_divider_end=
### pc_split_divider= \315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315
### pc_split_divider_start=
### pc_split_divider_end=
enter_ca_mode-24_lines_per_page=\E\\1
ignore-24_lines_per_page=\E\\1
not_imp-48_lines_per_page=\E\\2
not_imp-96_lines_per_page=\E\\3
##--- INIT ---##
#################################
##### ##CSI next line commented
##### ##enter_ca_mode=^O\Ew\E\\1
#################################
### enter_ca_mode=
enter_ca_mode-auto_page_disable=\Ew
pass-.........auto_page_disable=\Ew
not_imp-auto_page_enable.......=\Ev
enter_ca_mode-xon_xoff_enable=^O
pass-.........xon_xoff_enable=^O
not_imp-xon_xoff_disable=^N
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
mode-00000000-00000002=\Ed$<20/>
mode-00000002-00000000=\Eb$<20/>
pass-screen_display_on=\En
not_imp-screen_display_off=\Eo
ignore-keyboard_disable=\E#
pass-..keyboard_enable.=\E"
pass-select_duplex_mode=\EDF
pass-enable_conversational_mode=\EC
not_imp-block_mode=\EB
pass-line_edit_mode=\EO
not_imp-page_edit_mode=\EN
pass-monitor_mode_disable=\EX
pass-monitor_mode_disable=\Eu
not_imp-monitor_mode_enable=\EU
pass-local_edit_mode_disable=\El
ignore-local_edit_mode_enable=\Ek
pass-key_click_enable=\E>
pass-key_click_disable=\E<
pass-smooth_scroll_enable=\E8
pass-smooth_scroll_disable=\E9
not_imp-field_tab=\Ei
pass-...load_insert_char_with_space...=\Ee\040
not_imp-load_insert_char..............=\Ee%p1%c
not_imp-load_print_ack_char=\E^F%p1%c
not_imp-local_mode=\Ec
pass_current-select_print_buffer_size=\E^J%p1%c
pass_current-program_print_boundaries=\E^P%p1%c
pass_current-formatted_print.........=\EP
pass_current-unformatted_print.......=\EL
pass_current-set_send_delimiters=\Ex0%p1%c
pass_current-set_send_delimiters=\Ex1%p1%c
pass_current-set_send_delimiters=\Ex2%p1%c
pass_current-set_send_delimiters=\Ex3%p1%c
pass_current-set_send_delimiters=\Ex4%p1%c
not_imp-send_form_all...........=\E^R
not_imp-send_line_all...........=\E6
not_imp-send_line_unprotected...=\E4
not_imp-send_message_all........=\Es
not_imp-send_message_unprotected=\ES
not_imp-send_page_all...........=\E7
not_imp-send_page_unprotected...=\E5
not_imp-send_25th_line..........=\EZ%p1%c
not_imp-auto_scroll_enable_disable=\EH
not_imp-bell_tone_enable=\E_
not_imp-bell_tone_disable=\E\^
not_imp-configure_main_port=\E}%p1%c%p2%c%p3%c%p4%c
not_imp-configure_aux_port.=\E{%p1%c%p2%c%p3%c%p4%c
not_imp-display_control_char=\EF%p1%c
not_imp-display_tab_status=\E^T
not_imp-DTR_handshake_disable=\E^?
not_imp-DTR_handshake_enable=\E~
not_imp-......enable_line_lock..=\E!1
pass_current-.disable_line_lock.=\E!2
not_imp-load_answerback_msg=\E^E%80S1
pass_current-send_answerback_msg=^E
not_imp-load_setup_parameters=\E^B
not_imp-load_xon_char.=\E^Q%p1%c
not_imp-load_xoff_char=\E^S%p1%c
not_imp-reprogram_unshifted_send_key=\E01%p1%c
not_imp-reprogram_shifted_send_key..=\E02%p1%c
not_imp-reprogram_unknown_send_key..=\E0%p2%c%p1%c
##### >>> pass=\E016
##### >>> pass=\E004
not_imp-read_cursor_address=\E/
not_imp-read_cursor_character=\E^C
not_imp-read_cursor_position=\E?
not_imp-read_cursor_position=\E^_
not_imp-save_restore_cursor_address=\E^U%p1%c
not_imp-select_data_entry_attribute=\E^D%p1%c
not_imp-select_write_protect_attribute=\Ep%p1%c
not_imp-send_set_up_parameters=\E^A
not_imp-send_terminal_id=\EM
not_imp-tab_by_word=\E^W%p1%c
not_imp-25th_line_disable=\E^N
not_imp-25th_line_enable.=\E^O
###############################
## FacetTerm Menu Capabilities for a freedom one 07/30/91
## Copyright (c) Structured Software Solutions, Inc. 1991.  All Rights Reserved.
## @(#) $Header: f1.FS,v 70.1 92/03/09 16:52:29 ssa Exp $
## ##-menu-cap-init_string=
##-menu-cap-cursor_address=\E=%p1%' '%+%c%p2%' '%+%c
##-menu-cap-clear_screen=\E*
##-menu-cap-clr_eol=\ET
## ##-menu-cap-ena_acs=
##-menu-cap-enter_alt_charset_mode=\E$
##-menu-cap-exit_alt_charset_mode=\E%
##-menu-cap-acs_chars=jHkGlFmEqKtMuLvOwNxJ
## ##-menu-cap-keypad_xmit=
##-menu-cap-no_attr=\EG0
##-menu-cap-shadow_attr=\EG0
##-menu-cap-item_attr=\EG0
##-menu-cap-box_attr=\EGD
##-menu-cap-title_attr=\EGD
##-menu-cap-highlight_attr=\EG4
##-menu-cap-highlight_blink_attr=\EG6
## ##-menu-cap-magic_cookie=0
## ##-menu-cap-ceol_standout_glitch
##-menu-cap-key_up=^K
##-menu-cap-key_down=^V
##-menu-cap-key_right=^L
## ##-menu-cap-key_left=
## ##-menu-cap-key_ignore=
###############################
