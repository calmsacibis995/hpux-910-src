# FacetTerm  wy60-25pc  (80 column only)  description file  08/21/89
#    WYSE 60 in 25 line native mode switchable to PC-TERM mode for VPIX.
#    Uses 3 screen pages.
#
#     !!! IMPORTANT NOTE !!!
#     Set Terminal to:   DATA BITS=8/1       PARITY=NONE
#     Set Terminal to:   XMT HNDSHAKE=NONE   XPC HNDSHAKE=ON
#
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: wy60-25pc.FS,v 70.1 92/03/09 16:51:00 ssa Exp $
### xon_xoff
xon_xoff
### graphics_escape_control
graphics_escape_control
### graphics_escape_delete
### columns=80
columns=80
### columns_wide=132
### columns_wide_mode_on_default
### columns_wide_on=
### columns_wide_off=
pass-columns_wide_off=\E`:
### columns_wide_clears_screen
### columns_wide_mode_on_mode_bit-08000000
### parm_mode_val-COLSWIDE-00000000=
### lines=24
lines=25
##--- MARGINS ---##
### auto_left_margin
auto_left_margin
### eat_newline_glitch
### real_eat_newline_glitch
### auto_wrap_on=
auto_wrap_on=\Ed/
### auto_wrap_off=
auto_wrap_off=\Ed.
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
### NOTE: clear all unprot to spaces(noattr)-leaves protect attr graph modes-
clear_screen_unprotected--=^Z
### clear_screen_unprotected_w_attr--=
### clear_screen--P=
### NOTE: clear all to nulls - protect off - attr off - graphics off
clear_screen--URBDIPA=\E+
clear_screen--URBDIPA=\E*
### clr_eos=
clr_eos=\Ey
clr_eos=\EY
### out_clr_eos=
### clr_eos_w_attr=
### clr_eos_chars=
### set_attr_eos=
### clr_eos_chars_erasable-=
### clear_all=
### clear_all_chars_erasable-=
### clr_eol=
clr_eol=\Et
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
cursor_address_wide=\Ea%i%p1%dR%p2%dC
### cursor_address_wide_starts=number
cursor_address_wide_starts=80
### row_address=
### column_address=
### decode_type-0123456789;?=\E[
### cursor_home=
cursor_home=\E{
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
##--- SCROLL ---##
### change_scroll_region=
### scroll_forward=
### scroll_could_be_cursor_only
scroll_could_be_cursor_only
### parm_index=
### scroll_reverse=
scroll_reverse=\Ej
### scroll_reverse_move_col0
### parm_rindex=
### save_cursor=
### restore_cursor=
### auto_scroll_on=
auto_scroll_on=\EO
### auto_scroll_off=
auto_scroll_off=\EN
### auto_scroll_off_wraps_top
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
##--- ATTRIBUTES URBDOIPA ---##
### attribute-U=
### attribute--U=
### attribute-R=
### attribute-B=
### attribute-D-O=
### attribute-O-D=
attribute--URBDI=\EG0
attribute-D-URBI=\EGp
attribute-BD-URI=\EGr
attribute-RD-UBI=\EGt
attribute-I-URBD=\EG1
attribute-UD-RBI=\EGx
attribute-B-URDI=\EG2
attribute-RBD-UI=\EGv
attribute-UBD-RI=\EGz
attribute-R-UBDI=\EG4
attribute-U-RBDI=\EG8
attribute-RB-UDI=\EG6
attribute-UB-RDI=\EG:
attribute-UR-BDI=\EG<
attribute-URD-BI=\EG|
attribute-URB-DI=\EG>
attribute-URBD-I=\EG~
### attribute-I=
### NOTE: attribute-I=\EG1
### attribute-P=
attribute-P=\E)
attribute--P=\E(
### attribute-A=
attribute-A=\EH^B
### attribute--A=
attribute--A=\EH^C
### attribute_next-A-=
attribute_next-A-=\EH
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
cursor_on=\E`1
### cursor_off=
cursor_off=\E`0
### cursor_on_mode_bit-00800000
### parm_mode_val-CURSORON-00000000=
### cursor_type=
cursor_type=\E`5
cursor_type=\E`3
cursor_type=\E`2
cursor_type=\E`4
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
##--- TABS ---##
### clear_all_tabs=
clear_all_tabs=\E0
### set_tab=
set_tab=\E1
### back_tab=
pass-clear_tab=\E3
##--- FUNCTION KEYS ---##
use=wysekeys.fc
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
key_home=^^
### key_up=
key_up=^K
### key_down=
### key_right=
key_right=^L
### key_left=
### key_backspace=
##--- PRINTER ---##
### transparent_print_on=
### transparent_print_off=
### # TBD print_screen=
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
### NOTE: NOT_IN_25 status_off=\EA11
### NOTE: NOT_IN_25 status_line=\Ef%s1\r
##--- MISC ---##
### # ONLY IF PADDING carriage_return=\r$<20/>
### # ONLY IF NOT \n  new_line=
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
pc_mode_on=\E~5
### pc_mode_off=
pc_mode_off=\Ev4
### pc_mode_kd_scancode_driver
### out_pc_mode_on_1=
###       NOTE: default_attr_dim=\E`7 screen_off=\E`8 pc_mode=\E~5
out_pc_mode_on_1=\E`7\E`8\E~5$<2000>
### out_pc_mode_on_2=
###       NOTE: screen_off=\E`8 auto_scroll_on=\EO
out_pc_mode_on_2=\EO
### out_pc_mode_on_3=
###       NOTE: pc_mode_enhanced=\Ev! screen_on=\E`9 no_scroll=\EN
out_pc_mode_on_3=\Ev!\EN
### out_pc_mode_off_1=
###       NOTE: screen_off wy60_mode=\Ev4
out_pc_mode_off_1=\EO\Ev4$<6>
### out_pc_mode_off_2=
###       NOTE: screen_off=\E`8  auto_page_off=\Ed* 
###       NOTE: select_bank_2=\EcB2  
###       NOTE: page_1=\E-1\040\040  set=\EcD
###       NOTE: page_2=\E-2\040\040  set=\EcD
###       NOTE: page_0=\E-0\040\040  set=\EcD
out_pc_mode_off_2=\E`8\Ed*\EcB2\E-1\040\040\EcD\E-2\040\040\EcD\E-0\040\040\EcD
### out_pc_mode_off_3=
###       NOTE: screen on=\E`9
out_pc_mode_off_3=\E`9
### pc_mode_se_on_switch_only
### pc_mode_does_clear_screen
### pc_mode_on_turns_auto_wrap_off
pc_mode_on_turns_auto_wrap_off
### pc_mode_off_turns_auto_wrap_on
### pc_mode_on_switch_page_number_0
pc_mode_on_switch_page_number_0
### pc_mode_off_switch_page_number_0
pc_mode_off_switch_page_number_0
### pc_auto_wrap_on=
pc_auto_wrap_on=\E~
### pc_auto_wrap_off=
pc_auto_wrap_off=\E0
### pc_cursor_on=
pc_cursor_on=\E.1
### pc_cursor_off=
pc_cursor_off=\E.0
### pc_cursor_type=
pc_cursor_type=\E.5
pc_cursor_type=\E.3
pc_cursor_type=\E.2
pc_cursor_type=\E.4
### pc_cursor_type=
##--- PASTE ---##
### paste_eol-type=
##--- GRAPH_MODE ---##
### graph_mode_start=
### graph_mode_clear=
### out_graph_mode_clear=
##--- MULTIPAGE ---##
### pages=number
pages=3
### display_page_number=
display_page_number=\E-%p1%d\040\040
### parm_display_page_next=
### parm_display_page_prev=
### display_page_next=
### NOTE: display_page_next=\EK
### display_page_prev=
### NOTE: display_page_prev=\EJ
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
### NOTE: xon-xoff=\Ec21  noxmitxonxoff=\Ec40   ack_mode_off=\Ee6
### NOTE: 25_line\Ee(   econ80=\EeG   autopageoff=\Ed*   autowrap=\Ed/  
### NOTE: autoscroll=\EO
enter_ca_mode=\Ec21\Ec40\Ee6\Ee)\EeG\Ed*\Ed/\EO
### enter_ca_mode_pc=
### NOTE: auto_font_off=\EeN  pc_in_bank_0=\Ec@0D
### NOTE: mul_bank_1=\Ec@1A   native_bank_2=\Ec@2@   mul_bank_3=\Ec@3a
enter_ca_mode_pc=\EeN\Ec@0D\Ec@1A\Ec@2@\Ec@3A
### enter_ca_mode_pc_2=
###       NOTE: select_bank_2=\EcB2
###       NOTE: page_1=\E-1\040\040  set=\EcD
###       NOTE: page_2=\E-2\040\040  set=\EcD
###       NOTE: page_0=\E-0\040\040  set=\EcD
enter_ca_mode_pc_2=\EcB2\E-1\040\040\EcD\E-2\040\040\EcD\E-0\040\040\EcD
### enter_ca_mode_2=
### exit_ca_mode=
### exit_ca_mode_pc=
###       NOTE: auto_font_bank_on=\EeO  init_tabs_on=\E;  wy60=\E~4
exit_ca_mode_pc=\EeO\Ee;\E~4
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
ignore-recv_xon_xoff=\Ec21
ignore-xmit_no_xon_xoff=\Ec40
ignore-xmit_xon_xoff=\Ec41
ignore-25_line=\Ee)
ignore-24_line=\Ee(
ignore-auto_page_off=\Ed*
ignore-wyse_60_mode=\E~4
ignore-ack_mode_off=\Ee6
### SPLITUP init_2string=\Ec21\Ee)\
### SPLITUP reset_file=/usr/lib/tabset/vt100
### reverse video
mode-00000000-00000001=\EA00$<20/>
mode-00000001-00000000=\EA04$<20/>
### screen off
mode-00000000-00000002=\E`9$<20/>
mode-00000002-00000000=\E`8$<20/>
pass-local_message_field_blank-=\EA21
pass-host_message_field=\EF%s1\r
pass-protect_char_reverse=\E`6
pass-protect_char_dim....=\E`7
pass-protect_char_normal.=\E`A
ignore-keyboard_lock=\E#
pass-keyboard_unlock=\E"
pass-conversational_mode=\EC
not_imp-block_mode=\EB
pass-full_duplex=\EDF
not_imp-half_duplex=\EDH
pass-monitor_off=\Eu
not_imp-monitor_on=\EU
### PCTERM enhance on
ignore-pcterm_enhance_on=\Ev!
### NOTE: key_clear=\EY
use=wy60.fm
