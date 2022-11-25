# FacetTerm  pcterm  (80 column only)  description file 05/18/89
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: pcterm.FS,v 70.1 92/03/09 16:01:59 ssa Exp $
xon_xoff
columns=80
graphics_8_bit
### NOT_IN_80 columns_wide_off=\E`:
pass=\E`:
lines=25
use=wy50prnt.fc
pass-program_left_shift=\E|/1\EZ^Y
## FacetTerm  wy50com  description file 10/09/88
### NOT_IN_COMMON columns=80
### NOT_IN_COMMON columns_wide=132
### NOT_IN_COMMON columns_wide_on=\E`;
### NOT_IN_COMMON columns_wide_off=\E`:
### NO columns_wide_clears_screen
### NO parm_mode_val-COLSWIDE-00000000=?3
### NOT_IN_COMMON lines=24
### NO graphics_8_bit
### ?? graphics_escape_control
### ?? input_8_bit
### NOT_IN_COMMON xon_xoff
##--- MARGINS ---##
auto_left_margin
### ASSUMED auto_right_margin
### NO eat_newline_glitch
### NO auto_wrap_on=
### NO auto_wrap_off=
### ?? auto_wrap_mode_bit-04000000
### ?? parm_mode_val-AUTOWRAP-00000000=?7
##--- CLEAR ---##
### protect submode  off&autoscrollon=\E'  on&autoscrolloff=\E&
write_protect_on=\E&
write_protect_off=\E'
### clear all unprotected to nulls
clear_screen_unprotected--=\E:
### clear all unprotected to spaces
clear_screen_unprotected_w_attr--=^Z
clear_screen_unprotected_w_attr--=\E;
### clear all to nulls - protect off
clear_screen--P=\E+
clear_screen--P=\E*
clr_eol=\Et
clr_eol_w_attr=\ET
clr_eos=\Ey
clr_eos_w_attr=\EY
##--- CURSOR ---##
cursor_address=\E=%p1%' '%+%c%p2%' '%+%c
cursor_address_wide_starts=80
cursor_address_wide=\Ea%i%p1%dR%p2%dC
cursor_home=\E{
cursor_home=^^
cursor_up=^K
cursor_up_at_home_wraps_ll
### NO parm_up_cursor=
cursor_down=^V
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
scroll_could_be_cursor_only
### NO ## NOT_IMP parm_index=
scroll_reverse=\Ej
### NO ## NOT_IMP parm_rindex=
### NO save_cursor=
### NO restore_cursor=
### auto_scroll_on=\EO
### auto_scroll_off=\EN
### auto_scroll_off_wraps_top
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
### REVERSE VIDEO WITH PROTECT
attribute-P=\E)
attribute--P=\E(
### GRAPHICS
attribute-A=\EH^B
attribute--A=\EH^C
attribute_next-A-=\EH
attribute_next-I=^B
##--- MULTIPLE ATTRIBUTES URBDOIPA ---##
##---                              ---##
##--- MAGIC COOKIE ATTRIBUTES ---##
## magic=^B\030
## magic=^B\257
##--- CHARACTER SET ATTRIBUTES URBDOIPA ---##
### lock_shift_0=
select_character_set_0--URBD=\EG0
select_character_set_0-D-URB=\EGp
select_character_set_0-BD-UR=\EGr
select_character_set_0-RD-UB=\EGt
select_character_set_0-UD-RB=\EGx
select_character_set_0-B-URD=\EG2
select_character_set_0-RBD-U=\EGv
select_character_set_0-UBD-R=\EGz
select_character_set_0-R-UBD=\EG4
select_character_set_0-U-RBD=\EG8
select_character_set_0-RB-UD=\EG6
select_character_set_0-UB-RD=\EG:
select_character_set_0-UR-BD=\EG<
select_character_set_0-URD-B=\EG|
select_character_set_0-URB-D=\EG>
select_character_set_0-URBD-=\EG~
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
### NO ## TBD init_tabs=number
### NO ## ASSUMED tab=
back_tab=\EI
clear_all_tabs=\E0
set_tab=\E1
##--- FUNCTION KEYS ---##
### NOT_IN_COMMON use=wysekeys.fc
### ?? function_key_clear=
### ?? function_key_clear_mode_yes=number
### ?? function_key_clear_mode_no=number
### ?? parm_pkey_key=
### ?? parm_pkey_key=
### SEE wysekeys pkey_key=\Ez%p1%c%s1^?
### NO ## TBD pkey_local=
### NO ## TBD pkey_xmit=
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
### NOT_IN_COMMON AUX_PRINT_OFF prtr_off=^T
### TBD prtr_on=^R
##--- STATUS LINE ---##
### TBD has_status_line
### ?? ## TBD status_line_esc_ok
### TBD width_status_line=number
### TBD dis_status_line=
### NO status_off=
### NO status_off_clears_status_type
status_type_turns_status_on
status_type-normal=\EA10
status_type-blank=\EA11
status_type-underline=\EA18
status_type-reverse=\EA14
status_type-blink=\EA12
status_type-dim=\EA1p
status_type-reverse_blink=\EA16
status_type-dim_reverse=\EA1t
status_type-dim_blink=\EA1r
status_type-dim_reverse_blink=\EA1v
status_type-underscore_blink=\EA1:
status_type-underscore_reverse=\EA1<
status_type-underscore_reverse_blink=\EA1>
status_type-dim_underscore=\EA1x
status_type-dim_underscore_blink=\EA1z
status_type-dim_underscore_reverse=\EA1|
status_type-dim_underscore_reverse_blink=\EA1~
status_type_ruins_status_line
### COMBINE to_status_line=\Ef
### COMBINE from_status_line=\r
status_line=\Ef%78s1\r
status_line=\Ez(%78s1\r
status_line_turns_status_on
*status_line_clears_status_line_labels
*status_off_clears_status_line_labels
### *multi_status_line_labels-01234567=\Ez%p1%c%s1\r
*status_line_label=\Ez0%8s1\r
*status_line_label=\Ez1%8s1\r
*status_line_label=\Ez2%8s1\r
*status_line_label=\Ez3%8s1\r
*status_line_label=\Ez4%8s1\r
*status_line_label=\Ez5%8s1\r
*status_line_label=\Ez6%8s1\r
*status_line_label=\Ez7%8s1\r
*not_imp-other_labels=\Ez8%8s1\r
*not_imp-other_labels=\Ez9%8s1\r
*not_imp-other_labels=\Ez:%8s1\r
*not_imp-other_labels=\Ez;%8s1\r
*not_imp-other_labels=\Ez<%8s1\r
*not_imp-other_labels=\Ez=%8s1\r
*not_imp-other_labels=\Ez>%8s1\r
*not_imp-other_labels=\Ez?%8s1\r
*not_imp-other_labels=\EzP%8s1\r
*not_imp-other_labels=\EzQ%8s1\r
*not_imp-other_labels=\EzR%8s1\r
*not_imp-other_labels=\EzS%8s1\r
*not_imp-other_labels=\EzT%8s1\r
*not_imp-other_labels=\EzU%8s1\r
*not_imp-other_labels=\EzV%8s1\r
*not_imp-other_labels=\EzW%8s1\r
*not_imp-other_labels=\EzX%8s1\r
*not_imp-other_labels=\EzY%8s1\r
*not_imp-other_labels=\EzZ%8s1\r
*not_imp-other_labels=\Ez[%8s1\r
*not_imp-other_labels=\Ez\\%8s1\r
*not_imp-other_labels=\Ez]%8s1\r
*not_imp-other_labels=\Ez\^%8s1\r
*not_imp-other_labels=\Ez_%8s1\r
### *not_imp-other_labels=\Ez%p1%c%79s1\r
pass-shifted_label_off=\Ez^?
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
### NOT_IN_COMMON pages=number
### NOT_IN_COMMON display_page_number=
### NOT_IN_COMMON parm_display_page_next=
### NOT_IN_COMMON parm_display_page_prev=
### NOT_IN_COMMON display_page_next=
### NOT_IN_COMMON display_page_prev=
##--- SPLIT SCREEN ---##
*split_divider= ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
*split_divider_start=\EH^B
*split_divider_end=\EH^C
##--- INIT ---##
enter_ca_mode=\EO
pass=\EO
### NOT_IN_COMMON exit_ca_mode=
### protect_submode_off protect_off
### SPLITUP init_1string=\E'\E(
### protect submode  off&autoscrollon=\E'  on&autoscrolloff=\E&
### pass=\E'
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
mode-00000000-00000002=\EA00$<20/>
mode-00000002-00000000=\EA04$<20/>
### local message field 
pass-local_msg_field_attr=\EA2%p1%c
### host message field
pass-host_msg_field_attr=\EA3%p1%c
pass-host_msg_field_msg=\EF%46s1\r
### reverse protect character   A=normal 7=dim
multi_mode-0000000C-00000000=\E`6$<20>
multi_mode-0000000C-00000004=\E`A$<20>
multi_mode-0000000C-00000008=\E`7$<20>
### keyboard unlock=\E" lock=\E# unlock=^N
not_imp-lock_keyboard=\E#
pass-unlock_keyboard=\E"
pass-unlock_keyboard=^N
### mode conversational=\EC     block=\EB
pass-conversational=\EC
### duplex full=\EDF half=\EDH
pass-full_duplex=\EDF
### monitor off=\Eu on=\EU off=\EX
pass-monitor_off=\Eu
pass-monitor_off=\EX
### edit submode  duplex=\El  local=\Ek
pass-duplex_edit=\El
### smooth scroll  jump=\E`@ smooth1=\E`< smooth2=\E`= smooth4=\E`> smooth8=\E`?
pass-smooth_scroll_1=\E`<
pass-smooth_scroll_2=\E`=
pass-smooth_scroll_4=\E`>
pass-smooth_scroll_8=\E`?
pass-jump_scroll=\E`@
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
## FacetTerm  wysekeys  description file 12/08/87
pkey_key=\Ez%p1%c%s1^?$<80/>
function_key-@=^A@\r
function_key-A=^AA\r
function_key-B=^AB\r
function_key-C=^AC\r
function_key-D=^AD\r
function_key-E=^AE\r
function_key-F=^AF\r
function_key-G=^AG\r
function_key-H=^AH\r
function_key-I=^AI\r
function_key-J=^AJ\r
function_key-K=^AK\r
function_key-L=^AL\r
function_key-M=^AM\r
function_key-N=^AN\r
function_key-O=^AO\r
shift_function_key
function_key-`=^A`\r
function_key-a=^Aa\r
function_key-b=^Ab\r
function_key-c=^Ac\r
function_key-d=^Ad\r
function_key-e=^Ae\r
function_key-f=^Af\r
function_key-g=^Ag\r
function_key-h=^Ah\r
function_key-i=^Ai\r
function_key-j=^Aj\r
function_key-k=^Ak\r
function_key-l=^Al\r
function_key-m=^Am\r
function_key-n=^An\r
function_key-o=^Ao\r
use=pcterm.fm
