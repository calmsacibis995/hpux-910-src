# FACET/TERM  wyse60  (80 column only)  description file  06/14/90
#     WYSE 60 in 24 line native mode using 3 screen pages.
## @(#) $Header: wyse60.FS,v 66.6 90/09/20 12:57:05 kb Exp $
use=wy60ans.fc
use=wy60.fm
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
### NOTE columns_wide_on=\E`;
### columns_wide_off=
pass-columns_wide_off=\E`:
### columns_wide_clears_screen
### columns_wide_mode_on_mode_bit-08000000
### parm_mode_val-COLSWIDE-00000000=
### lines=24
lines=24
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
### NOTE: protect submode  off&autoscrollon=\E'  on&autoscrolloff=\E&
### write_protect_on=
write_protect_on=\E&
### write_protect_off=
write_protect_off=\E'
### clear_screen_unprotected--=
### NOTE: clear_all_unprotected_to_nulls=\E:
clear_screen_unprotected--=\E:
### NOTE: clear all unprot to spaces(noattr)-leaves protect attr graph modes-
clear_screen_unprotected--=^Z
clear_screen_unprotected--=\E;
### clear_screen_unprotected_w_attr--=
### clear_screen--P=
### NOTE: clear all to nulls - protect off - attr off - graphics off
clear_screen--URBDIPA=\E+
clear_screen--URBDIPA=\E*
### NOTE clear_screen=\E*$<100>
### clr_eos=
clr_eos=\Ey
clr_eos=\EY
### NOTE clr_eos=\Ey$<100>
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
### cursor_report
answer-..read_cursor_80.=%p1%' '%+%c%p2%' '%+%c\r
question-...............=\E?
answer-..read_cursor_132=%i%p1%dR%p2%dC
question-.............. =\Eb
### row_address=
row_address=\E[%p1%' '%+%c
### column_address=
### decode_type-0123456789;?=\E[
### cursor_home=
cursor_home=\E{
cursor_home=^^
### cursor_up=
cursor_up=^K
### cursor_up_at_home_wraps_ll
cursor_up_at_home_wraps_ll
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
### NOTE scroll_forward=\n$<5>
### scroll_could_be_cursor_only
scroll_could_be_cursor_only
### parm_index=
### scroll_reverse=
scroll_reverse=\Ej
### NOTE scroll_reverse=\Ej$<7>
### scroll_reverse_move_col0
### parm_rindex=
### save_cursor=
### restore_cursor=
### auto_scroll_on=
auto_scroll_on=\EO
### auto_scroll_off=
auto_scroll_off=\EN
### auto_scroll_off_wraps_top
auto_scroll_off_wraps_top
##--- INS/DEL ---##
### insert_line=
insert_line=\EE
### NOTE insert_line=\EE$<4>
### out_insert_line=
### insdel_line_move_col0
insdel_line_move_col0
### insert_line_needs_clear_glitch
### insert_line_sets_attributes
### parm_insert_line=
### out_parm_insert_line=
### delete_line=
delete_line=\ER
### NOTE delete_line=\ER$<5>
### parm_delete_line=
### insert_character=
insert_character=\EQ
### insert_padding=
### NOTE insert_padding=$<3>
### parm_ich=
### enter_insert_mode=
enter_insert_mode=\Eq
### exit_insert_mode=
exit_insert_mode=\Er
### insert_mode_on_mode_bit-02000000
### parm_mode_val-INSERTON-00000000=
### delete_character=
delete_character=\EW
### NOTE delete_character=\EW$<10>
### parm_delete_character=
##--- ATTRIBUTES URBDOIPA ---##
### attribute-U=
### attribute--U=
### attribute-R=
### attribute-B=
### attribute-D-O=
### attribute-O-D=
### attribute-I=
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
### NOTE set_attributes=%?%p8%t\E)%e\E(%;%?%p9%t\EH^B%e\EH^C%;\EG%'0'%?%p2%p6%|%t%{8}%|%;%?%p1%p3%|%p6%|%t%{4}%|%;%?%p4%t%{2}%|%;%?%p1%p5%|%t%{64}%|%;%?%p7%t%{1}%|%;%c
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
## download_chars=\EcA bank pp bb...bb ^Y
pass-download_chars=\EcA%p1%c%35s1^Y
### vt220 is ( )
### select_character_set_noload
select_character_set_noload
### lock_shift_0=
lock_shift_0=\EcD
### single_shift_0=
### select_character_set_0--=
select_character_set_0-...-=\EcB0
select_character_set_0-..a-=\EcB1
select_character_set_0-.b.-=\EcB2
select_character_set_0-.ba-=\EcB3
### select_character_set_0--=
### lock_shift_1=
lock_shift_1=\EcE
### single_shift_1=
### select_character_set_1--=
select_character_set_1-c..-=\EcC1
select_character_set_1-c.a-=\EcC2
select_character_set_1-cb.-=\EcC3
select_character_set_1-cba-=\EcC0
### select_character_set_1--=
### lock_shift_2=
### single_shift_2=
### select_character_set_2--=
### select_character_set_2--=
### lock_shift_3=
### single_shift_3=
### select_character_set_3--=
### select_character_set_3--=
### NOTE acs_chars=0wa_h[jukslrmqnxqzttuyv]wpxv
perwindow-5-CIQL----font.bank.0........=\Ec@0%p1%c
perwindow_default-native...............=\Ec@0@
perwindow-5-CIQL----font.bank.1........=\Ec@1%p1%c
perwindow_default-multinational........=\Ec@1A
perwindow-5-CIQL----font.bank.2........=\Ec@2%p1%c
perwindow_default-44.line.native.......=\Ec@2`
perwindow-5-CIQL----font.bank.3........=\Ec@3%p1%c
perwindow_default-44.line.multinational=\Ec@3a
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
clear_all_tabs=\E3
clear_all_tabs=\E0
### set_tab=
set_tab=\E1
### back_tab=
back_tab=\EI
### NOTE tab=\t$<1>
##--- FUNCTION KEYS ---##
use=wysekeys.fc
use=wyseedit.fc
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
### NOTE: key_clear=\EY
### NOTE key_ic=\EQ
### NOTE key_dc=\EW
### NOTE key_il=\EE
### NOTE key_dl=\ER
### NOTE key_npage=\EK
### NOTE key_ppage=\EJ
##--- PRINTER ---##
### transparent_print_on=
transparent_print_on=\Ed#
not_imp-aux_print_on=^R
### transparent_print_off=
transparent_print_off=^T
### # TBD print_screen=
pass-print_screen=\EP
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
use=wyse60stat.fc
### out_screen_saver_start=
out_screen_saver_start=\Ez(\r
### out_screen_saver_end=
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
pages=3
### display_page_number=
display_page_number=\E-%p1%d\040\040
### parm_display_page_next=
### parm_display_page_prev=
### display_page_next=
### NOTE: display_page_next=\EK
### display_page_prev=
### NOTE: display_page_prev=\EJ
ignore-select_page_0=\Ew0
ignore-select_page_1=\Ew1
ignore-divide_pages=\EwG
ignore-econ80_off=\EeF
##--- SPLIT SCREEN ---##
### NOTE: split_divider must be 77 char starting with blank
### split_divider= qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq
split_divider= \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
### NOTE: split_divider_end must reverse split_divider_start to default
### split_divider_start=
split_divider_start=\EH^B
### split_divider_end=
split_divider_end=\EH^C
### pc_split_divider= \315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315
### pc_split_divider_start=
### pc_split_divider_end=
##--- INIT ---##
### enter_ca_mode=
### wy60=\E~4 xon-xoff=\Ec21 ack_mode_off=\Ee6 24_line=\Ee(
###      econ80=\EeG autopageoff=\Ed* autowrap=\Ed/ autoscroll=\EO
enter_ca_mode=\E~4\Ec21\Ee6\Ee(\EeG\Ed*\Ed/\EO
ignore-forced-wy60=\E~4
pass-xon_xoff=\Ec21
pass-ack_mode_off=\Ee6
not_imp-25_line=\Ee)
ignore-forced-24_line=\Ee(
ignore-forced-auto_page_mode_off=\Ed*
### enter_ca_mode_pc=
### enter_ca_mode_pc_2=
### enter_ca_mode_2=
### exit_ca_mode=
### exit_ca_mode_pc=
### exit_ca_mode_pc_2=
### exit_ca_mode_2=
### NOTE init_2string=\Ec21\Ee)\
### NOTE reset_file=/usr/lib/tabset/vt100
### NOTE reset_1string=\E~!\E~4$<150>
### NOTE reset_2string=\EeF$<150>
### NOTE reset_3string=\EwG\Ee($<150>
### NOTE init_1string=\EcB0\EcC1
### NOTE init_2string=\Ed$\EcD\E'\Er\EH^C\Ed/\EO\Ee1\Ed*\E`@\E`9\E`1^N^T
### NOTE init_3string=^N^T
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
### reverse video
mode-00000000-00000001=\EA00$<20/>
mode-00000001-00000000=\EA04$<20/>
## screen_on=\E`9 screen_off=\E`8
mode-00000000-00000002=\E`9$<20/>
mode-00000002-00000000=\E`8$<20/>
## caps_lock_off=\Ee' caps_lock_on=\Ee&
mode-00000000-00000100=\Ee'
mode-00000100-00000000=\Ee&
### wyse50stat pass-local_message_field_blank=\EA21
### wyse50stat pass-host_message_field=\EF%s1\r
### TBD
pass-protect_normal=\E`A
pass-protect_reverse=\E`6
pass-protect_dim=\E`7
not_imp-lock_keyboard=\E#
pass-unlock_keyboard=\E"
pass-unlock_keyboard=^N
not_imp-block_mode=\EB
pass-conversational=\EC
not_imp-half_duplex=\EDH
pass-full_duplex=\EDF
### not_imp-monitor_on=\EU
use=wysemon.fc
pass-monitor_off=\Eu
pass-monitor_off=\EX
####################################################
## bidirectional_mode_off=\Ed$
pass-bidirectional_mode_off=\Ed\044
pass-bidirectional_mode_on=\Ed%%
### TBD character_attribute_mode_off=\Ee0
pass-character_attribute_mode_on=\Ee1
### TBD page_attribute_mode=\Ee2
### TBD line_attribute_mode=\Ee3
pass-jump_scroll=\E`@
pass-smooth_scroll_1=\E`<
pass-smooth_scroll_2=\E`=
pass-smooth_scroll_4=\E`>
pass-smooth_scroll_8=\E`?
pass-time_of_day=\Ec8%4f1
ignore-modem_port_transmit_xon_xoff=\Ec41
##### NOT_USED has_meta_key
##### NOT_USED move_insert_mode
##### NOT_USED move_standout_mode
##### NOT_USED label_height=1
##### NOT_USED label_width=8
##### NOT_USED num_labels=8
##### NOT_USED width_status_line=45
##### NOT_USED cursor_invisible=\E`0
##### NOT_USED cursor_normal=\E`1
##### NOT_USED cursor_to_ll=^^^K
##### NOT_USED dis_status_line=\EF\r
##### NOT_USED enter_dim_mode=\EGp
##### NOT_USED enter_protected_mode=\E)
##### NOT_USED enter_secure_mode=\EG1
##### NOT_USED enter_xon_mode=\Ec21
##### NOT_USED exit_xon_mode=\Ec20
##### NOT_USED key_btab=\EI
##### NOT_USED key_enter=\E7
##### NOT_USED key_eol=\ET
##### NOT_USED key_eos=\EY
##### NOT_USED key_print=\EP
##### NOT_USED key_replace=\Er
##### NOT_USED key_shome=\E{
##### NOT_USED label_off=\EA11
##### NOT_USED label_on=\EA10
##### NOT_USED pkey_local=\EZ2%p1%'?'%+%c%p2%s^?
##### NOT_USED pkey_xmit=\EZ1%p1%'?'%+%c%p2%s^?
##### NOT_USED plab_norm=\Ez%p1%'/'%+%c%p2%s\r
##### NOT_USED underline_char=\EG8%p1%c\EG0
ignore=^?
pass-enhance_mode_on=\E~!
## box from cursor to line and col
draw_box--2-A-:-A-3-A-6-A-5-A-1-A-=\EcG%p1%' '%+%c%p2%' '%+%c
## box from cursor to cursor plus width and height
draw_box_width_height--2-A-:-A-3-A-6-A-5-A-1-A-=\EcN%p1%' '%+%c%p2%' '%+%c
