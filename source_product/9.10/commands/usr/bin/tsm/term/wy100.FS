# FacetTerm  wy100  description file 05/02/88
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: wy100.FS,v 70.1 92/03/09 16:47:46 ssa Exp $
columns=80
### NOT WY100 columns_wide=132
### NOT WY100 columns_wide_on=\E`;
### NOT WY100 columns_wide_off=\E`:
### NO columns_wide_clears_screen
### ?? columns_wide_mode_on_mode_bit-08000000
### ?? parm_mode_val-COLSWIDE-00000000=?3
lines=24
### NO graphics_8_bit
### ?? graphics_escape_control
### ?? input_8_bit
xon_xoff
##--- MARGINS ---##
auto_left_margin
### ASSUMED auto_right_margin
### NO eat_newline_glitch
### NO auto_wrap_on=
### NO auto_wrap_off=
### ?? auto_wrap_mode_bit-04000000
### ?? parm_mode_val-AUTOWRAP-00000000=?7
##--- CLEAR ---##
### clear all unprotected to spaces
clear_screen--P=^Z
clear_screen--=\E;
### clear all to nulls - protect off
clear_screen--P=\E+
clear_screen--P=\E*
clr_eol=\Et
clr_eol_w_attr=\ET
clr_eos=\Ey
clr_eos=\EY
##--- CURSOR ---##
cursor_address=\E=%p1%' '%+%c%p2%' '%+%c
### NOT WY100 cursor_address_wide=\Ea%i%p1%dR%p2%dC
cursor_home=\E{
cursor_home=^^
cursor_up=^K
cursor_up_at_home_wraps_ll
### NO parm_up_cursor=
### NOT WY100 cursor_down=^V
### NO parm_down_cursor=
cursor_right=^L
### NO parm_right_cursor=
### SAME_AS_BACKSPACE cursor_left=^H
### NO parm_left_cursor=
### NO ## NOT_IMP column_address=
### NO ## NOT_IMP row_address=
### NO ## NOT_IMP cursor_mem_address=
### NO ## NOT_IMP cursor_to_ll=
##--- SCROLL ---##
### NO change_scroll_region=
### SAME_AS_NEWLINE scroll_forward=\n
### NO scroll_could_be_cursor_only
### NO ## NOT_IMP parm_index=
### NO scroll_reverse=
### NO ## NOT_IMP parm_rindex=
### NO save_cursor=
### NO restore_cursor=
##--- INS/DEL ---##
insert_line=\EE
insdel_line_move_col0
### NO parm_insert_line=
delete_line=\ER
### NO parm_delete_line=
insert_character=\EQ
### NO insert_padding=
### NO ## NOT_IMP parm_ich=
enter_insert_mode=\Eq
exit_insert_mode=\Er
### ?? insert_mode_on_mode_bit-02000000
### ?? parm_mode_val-INSERTON-00000000=4
delete_character=\EW
### NO parm_delete_character=
### NO ## NOT_IMP erase_chars=
### NO ## NOT_IMP enter_delete_mode=
### NO ## NOT_IMP exit_delete_mode=
### NO ## NOT_IMP repeat_char=
##--- ATTRIBUTES URBDOIPA ---##
### NO ## NOT_IMP set_attributes=
### NOTE magic_cookie_glitch=1
### USE_PROTECT attribute-?=\E`6\E)
### USE_PROTECT attribute--?=\E(
### NO attribute-U=
### NO attribute--U=
### NO attribute-R=
### NO attribute-B=
### NO attribute-D-O=
### NO attribute-O-D=
### NO attribute-I=
### REVERSE VIDEO WITH PROTECT
attribute-P=\E)
attribute--P=\E(
### GRAPHICS
### NOT WY100 attribute-A=\EH^B
### NOT WY100 attribute--A=\EH^C
attribute_next-A-=\EH
##--- MULTIPLE ATTRIBUTES URBDOIPA ---##
##---                              ---##
### ?? parm_attribute=\E[%p1%dm=
### ?? parm_attribute--URBDOIPA=\E[;%p1%dm
### ?? parm_attribute=\E[%p1%d;%p2%dm
### ?? parm_attribute--URBDOIPA=\E[;%p1%d;%p2%dm
### ?? parm_attribute_val--URBDOIPA=0
### ?? parm_attribute_val-U-=number
### NO exit_attribute_mode--URBDOIPA=
##--- MAGIC COOKIE ATTRIBUTES ---##
### USE_ATTR magic-standout=\E`6\E)
magic-underline=\EG8
magic-reverse=\EG4
magic-blink=\EG2
magic-dim=\EGp
### NO magic-bold=
### NO magic-secure=
### USE_ATTR magic-protected=
### USE_ATTR magic-alt_charset=
magic-reverse_blink=\EG6
magic-dim_reverse=\EGt
magic-dim_blink=\EGr
magic-dim_reverse_blink=\EGv
magic-underscore_blink=\EG:
magic-underscore_reverse=\EG<
magic-underscore_reverse_blink=\EG>
magic-dim_underscore=\EGx
magic-dim_underscore_blink=\EGz
magic-dim_underscore_reverse=\EG|
magic-dim_underscore_reverse_blink=\EG~
nomagic-exit_attribute_mode=\EG0
### USE_ATTR nomagic-exit_standout_mode=\E(
### NO nomagic-exit_underline_mode=
### NO nomagic-exit_alt_charset_mode=
##--- CHARACTER SET ATTRIBUTES URBDOIPA ---##
### NO lock_shift_0=
### NO select_character_set_0--=
### NO select_character_set_0--=
### NO lock_shift_1=
### NO select_character_set_1--=
### NO select_character_set_1--=
### NO single_shift_2=
### NO select_character_set_2--=
### NO single_shift_3=
### NO select_character_set_3--=
##--- LINE ATTRIBUTES ---##
### NO line_attribute_off=
### NO line_attribute=
### NO line_attribute=
##--- CURSOR (cursor_normal cursor_invisible cursor_visible)
cursor_on=\E`1
cursor_off=\E`0
### ?? cursor_on_mode_bit-00800000
### ?? parm_mode_val-CURSORON-00000000=?25
cursor_type=\E`5
cursor_type=\E`3
cursor_type=\E`2
cursor_type=\E`4
cursor_type_turns_cursor_on
##--- KEYPAD (keypad_xmit keypad_local ) ---##
### NO enter_appl_keypad_mode=
### NO exit_appl_keypad_mode=
### ?? appl_keypad_mode_on_mode_bit-00400000
### ?? parm_mode_val-APPKEYON-00000000=?4
### NO enter_cursor_key_mode=
### NO exit_cursor_key_mode=
### ?? cursor_key_mode_on_mode_bit-00200000
### ?? parm_mode_val-CURKEYON-00000000=?4
##--- TABS ---##
### NO ## ASSUMED tab=
back_tab=\EI
clear_all_tabs=\E0
set_tab=\E1
##--- FUNCTION KEYS ---##
pkey_key=\Ez%p1%c%s1^?$<80/>
function_key-@=^A@\r
function_key-A=^AA\r
function_key-B=^AB\r
function_key-C=^AC\r
function_key-D=^AD\r
function_key-E=^AE\r
function_key-F=^AF\r
function_key-G=^AG\r
shift_function_key
function_key-H=^AH\r
function_key-I=^AI\r
function_key-J=^AJ\r
function_key-K=^AK\r
function_key-L=^AL\r
function_key-M=^AM\r
function_key-N=^AN\r
function_key-O=^AO\r
### ?? function_key_clear=
### ?? function_key_clear_mode_yes=number
### ?? function_key_clear_mode_no=number
### ?? parm_pkey_key=
### ?? parm_pkey_key=
### USE_COMMON pkey_key=\Ez%p1%c%s1^?
### ?? function_key_string_is_hex
### ?? function_key-id-delim=
### ?? shift_function_key
### ?? function_key-id-delim=
### BY_HAND function_key-key_f0=^A@\r
### BY_HAND function_key-key_f1=^AA\r
### BY_HAND function_key-key_f2=^AB\r
### BY_HAND function_key-key_f3=^AC\r
### BY_HAND function_key-key_f4=^AD\r
### BY_HAND function_key-key_f5=^AE\r
### BY_HAND function_key-key_f6=^AF\r
### BY_HAND function_key-key_f7=^AG\r
### BY_HAND function_key-key_f8=^AH\r
### BY_HAND function_key-key_f9=^AI\r
### NO ## BY_HAND function_key-key_f10=
##--- CURSOR KEYS ---##
key_home=^^
key_up=^K
### SAME_AS_NEWLINE key_down=\n
### SAME_AS_BACKSPACE key_left=^H
key_right=^L
### SAME_AS_BACKSPACE key_backspace=^H
##--- PRINTER ---##
### TBD print_screen=\EP
### NO ## TBD prtr_non=
### TBD AUX_PRINT_NO_TRANSPARENT prtr_off=^T
### TBD prtr_on=^R
##--- STATUS LINE ---##
### TBD has_status_line
### ?? ## TBD status_line_esc_ok
### TBD width_status_line=number
### TBD dis_status_line=
### NO status_off=\EA11
### NO status_off_clears_status_type
status_type_turns_status_on
status_type=\EA10
status_type=\EA14
status_type_ruins_status_line
### COMBINE to_status_line=\Ef
### COMBINE from_status_line=\r
status_line=\Ef%s1\r
status_line_turns_status_on
##--- MISC ---##
### ASSUMED bell=^G
### ONLY IF PADDING carriage_return=\r
new_line=^_
### NO ## NOT_IMP form_feed=
### NO ## OK_IF_MAGIC transparent_underline
### NO ## NOT_IMP ceol_standout_glitch
### NO ## NOT_IMP erase_overstrike
### NO ## NOT_IMP generic_type
### NO ## NOT_IMP hard_copy
### NO ## NOT_IMP has_meta_key
### NO ## NOT_IMP insert_null_glitch
### NO ## NOT_IMP memory_above
### NO ## NOT_IMP memory_below
### NO ## NOT_IMP move_insert_mode
### NO ## NOT_IMP move_standout_mode
### NO ## NOT_IMP over_strike
### NO ## NOT_IMP tilde_glitch
### NO ## NOT_IMP lines_of_memory=number
### NO ## NOT_IMP padding_baud_rate=number
### NO ## NOT_IMP virtual_terminal=number
### NO ## NOT_IMP command_character=
### NO ## NOT_IMP down_half_line=
### NO ## NOT_IMP meta_off=
### NO ## NOT_IMP meta_on=
### NO ## NOT_IMP pad_char=
### NO ## NOT_IMP set_window=
### NO ## NOT_IMP underline_char=
### NO ## NOT_IMP up_half_line=
### NO ## NOT_USED key_a1=
### NO ## NOT_USED key_a3=
### NO ## NOT_USED key_b2=
### NO ## NOT_USED key_c1=
### NO ## NOT_USED key_c3=
### NO ## NOT_USED key_catab=
### NOT USED key_clear=\EY
### NO ## NOT_USED key_ctab=
### NOT_USED key_dc=\EW
### NOT_USED key_dl=\ER
### NO ## NOT_USED key_eic=
### NOT_USED key_eol=\ET
### NOT_USED key_eos=\EY
### NOT_USED key_ic=\EQ
### NOT_USED key_il=\EE
### NO ## NOT_USED key_ll=
### NOT_USED key_npage=\EK
### NOT_USED key_ppage=\EJ
### NO ## NOT_USED key_sf=
### NO ## NOT_USED key_sr=
### NO ## NOT_USED key_stab=
### NO ## NOT_USED lab_f0=
### NO ## NOT_USED lab_f1=
### NO ## NOT_USED lab_f2=
### NO ## NOT_USED lab_f3=
### NO ## NOT_USED lab_f4=
### NO ## NOT_USED lab_f5=
### NO ## NOT_USED lab_f6=
### NO ## NOT_USED lab_f7=
### NO ## NOT_USED lab_f8=
### NO ## NOT_USED lab_f9=
### NO ## NOT_USED lab_f10=
##--- MULTIPAGE ---##
### NO pages=number
### NO display_page_number=
### NO parm_display_page_next=
### NO parm_display_page_prev=
### NO display_page_next=
### NO display_page_prev=
##--- INIT ---##
### NO enter_ca_mode=
### NO exit_ca_mode=
### protect_submode_off protect_off
### SPLITUP init_1string=\E'\E(
### protect submode  off&autoscrollon=\E'  on&autoscrolloff=\E&
pass=\E'
### NO ## SPLITUP init_2string=
### NO ## SPLITUP init_3string=
### NO ## SPLITUP init_file=
### NO ## SPLITUP init_prog=
### NO ## SPLITUP reset_1string=
### NO ## SPLITUP reset_2string=
### NO ## SPLITUP reset_3string=
### NO ## SPLITUP reset_file=
##--- MODE ---##
### ?? mode-00000000-00000000=
### ?? ignore=
### ?? pass=
### screen on=\E`9   off=\E`8
mode-00000000-00000001=\E`9$<20>
mode-00000001-00000000=\E`8$<20>
### screen_reverse  normal=\EA00  reverse=\EA04
mode-00000000-00000002=\EA00$<20>
mode-00000002-00000000=\EA04$<20>
### local message field blank
pass=\EA21
### host message field
pass=\EF%s1\r
### reverse protect character   A=normal 7=dim
multi_mode-0000000C-00000000=\E`6$<20>
multi_mode-0000000C-00000004=\E`A$<20>
multi_mode-0000000C-00000008=\E`7$<20>
#### pass=\E`6
### keyboard unlock=\E" lock=\E#
ignore=\E#
pass=\E"
### mode conversational=\EC     block=\EB
pass=\EC
### duplex full=\EDF half=\EDH
pass=\EDF
### monitor off=\Eu on=\EU
pass=\Eu
### no scroll  off=\EO  on=\EN
pass=\EO
### edit submode  duplex=\El  local=\Ek
pass=\El
### data_validation off=\E> on=\E<
pass=\E>
##--- PARM_MODE ---##
### ?? parm_mode_on=\E[M?h
### ?? parm_mode_off=\E[M?l
### ?? parm_mode_private_propogates
ignore_mode_bit-80000000
### ?? parm_mode_val-80000000-IGNORE..=?99
pass_mode_bit-40000000
### ?? parm_mode_val-40000000-PASS....=?99
notimp_mode_bit-20000000
### ?? parm_mode_val-20000000-NOTIMP..=?99
use=wy100.fm
