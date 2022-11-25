# FacetTerm  ncr7930  (80 column only)  description file 09/26/89
#    NCR 7930 in 7901+ mode.
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: ncr7930.FS,v 70.1 92/03/09 16:49:49 ssa Exp $
columns=80
### ?? columns_wide=number
### ?? columns_wide_on=
### ?? columns_wide_off=
### ?? columns_wide_clears_screen
columns_wide_mode_on_mode_bit-08000000
### ?? parm_mode_val-COLSWIDE-00000000=?3
lines=24
### NO graphics_8_bit
### ?? graphics_escape_control
### ?? input_8_bit
### NO xon_xoff
##--- MARGINS ---##
### DOES TERMINAL HAVE auto_left_margin
### ASSUMED auto_right_margin
### NO eat_newline_glitch
### ?? auto_wrap_on=
### ?? auto_wrap_off=
auto_wrap_mode_bit-04000000
### ?? parm_mode_val-AUTOWRAP-00000000=?7
##--- CLEAR ---##
clear_screen=\EG
clear_screen=^L
### NO clear_all=^L
clr_eol=\EK
### out_clr_eol=\EK
### NO clr_eos_w_attr=\EK
### clr_eol_chars=\EK
### set_attr_eol=\EK
clr_eos=\Ek
### out_clr_eos=\Ek
### NO clr_eos_w_attr=
### clr_eos_chars=\Ek
### set_attr_eos=\Ek
##--- CURSOR ---##
cursor_address=\EY%p1%' '%+%c%p2%' '%+%c
### ?? cursor_address_wide=
cursor_home=^A
cursor_up=^Z
### ?? cursor_up_at_home_wraps_ll
### NO parm_up_cursor=
### SAME_AS_NEWLINE cursor_down=\n
### NO parm_down_cursor=
cursor_right=^F
### NO parm_right_cursor=
cursor_left=^U
### NO parm_left_cursor=
### NO ## NOT_IMP column_address=
### NO ## NOT_IMP row_address=
### NO ## NOT_IMP cursor_mem_address=
### NO ## NOT_IMP cursor_to_ll=
##--- SCROLL ---##
change_scroll_region=%i\Ep%p1%d;%p2%d\r
### SAME_AS_NEWLINE scroll_forward=\n
### ?? scroll_could_be_cursor_only
### NO ## NOT_IMP parm_index=
### NO scroll_reverse=
### NO ## NOT_IMP parm_rindex=
### NO save_cursor=
### NO restore_cursor=
##--- INS/DEL ---##
insert_line=\EM
### ?? insdel_line_move_col0
### NO parm_insert_line=
delete_line=\El
### NO parm_delete_line=
insert_character=\EF
### NO insert_padding=
### NO ## NOT_IMP parm_ich=
### NO enter_insert_mode=
### NO exit_insert_mode=
insert_mode_on_mode_bit-02000000
### ?? parm_mode_val-INSERTON-00000000=4
delete_character=\EE
### NO parm_delete_char=
### NO ## NOT_IMP erase_chars=
### NO ## NOT_IMP enter_delete_mode=
### NO ## NOT_IMP exit_delete_mode=
### NO ## NOT_IMP repeat_char=
##--- ATTRIBUTES URBDOIPA ---##
### NO ## NOT_IMP set_attributes=
### NOTE magic_cookie_glitch=0
### USE_CHARSET attribute-?=\E0P^N
### USE_CHARSET attribute--?=^O
### USE_CHARSET attribute-U=\E0P^N
### USE_CHARSET attribute--U=^O
### USE_CHARSET attribute-R=\E0P^N
### USE_CHARSET attribute-B=\E0B^N
### USE_CHARSET attribute-D-O=\E0A^N
### NO attribute-O-D=
### NO attribute-I=
### NO attribute-P=
### NO attribute-A=
### NO attribute--A=
### NO attribute_next-A-=
##--- MULTIPLE ATTRIBUTES URBDOIPA ---##
##---                              ---##
### ?? parm_attribute=\E[%7MMm
### ?? parm_attribute--URBDOIPA=\E[;%7MMm
### ?? parm_attribute_out=\E[%s1m
### ?? parm_attribute_out=\E[%s1;%s2m
### ?? parm_attribute_out=\E[%s1;%s2;%s3m
### ?? parm_attribute_out=\E[%s1;%s2;%s3;%s4m
### ?? parm_attribute_out=\E[%s1;%s2;%s3;%s4;%s5m
### ?? parm_attribute_out=\E[%s1;%s2;%s3;%s4;%s5;%s6m
### ?? parm_attribute_out=\E[%s1;%s2;%s3;%s4;%s5;%s6;%s7m
### ?? parm_attribute_out=\E[%s1;%s2;%s3;%s4;%s5;%s6;%s7;%s8m
### ?? parm_attribute_val--URBDOIPA=0
### ?? parm_attribute_val-U-=4
### ?? parm_attribute_val-R-=7
### ?? parm_attribute_val-B-=5
### ?? parm_attribute_val-D-O=2
### ?? parm_attribute_val-O-D=1
### ?? parm_attribute_val-I-=8
### ?? parm_attribute_val-P-=number
### ?? parm_attribute_val-A-=number
### USE_CHARSET exit_attribute_mode--URBDOIPA=^O
##--- MAGIC COOKIE ATTRIBUTES ---##
### USE_ATTR magic-standout=\E0P^N
### USE_ATTR magic-underline=\E0P^N
### USE_ATTR magic-reverse=\E0P^N
### USE_ATTR magic-blink=\E0B^N
### USE_ATTR magic-dim=\E0A^N
### NO ## USE_ATTR magic-bold=
### NO ## USE_ATTR magic-secure=
### NO ## USE_ATTR magic-protected=
### NO ## USE_ATTR magic-alt_charset=
### USE_ATTR nomagic-exit_attribute_mode=^O
### USE_ATTR nomagic-exit_standout_mode=^O
### USE_ATTR nomagic-exit_underline_mode=^O
### NO ## USE_ATTR nomagic-exit_alt_charset_mode=
##--- CHARACTER SET ATTRIBUTES URBDOIPA ---##
lock_shift_0=^O
select_character_set_0--=
lock_shift_1=^N
select_character_set_1--=\E0@
select_character_set_1-D-=\E0A
select_character_set_1-B-=\E0B
select_character_set_1-R-=\E0P
select_character_set_1-RB-=\E0R
select_character_set_1-U-=\E0`
### ?? single_shift_2=
### ?? select_character_set_2--=
### ?? single_shift_3=
### ?? select_character_set_3--=
##--- LINE ATTRIBUTES ---##
### ?? line_attribute_off=
### ?? line_attribute=
### ?? line_attribute=
##--- CURSOR (cursor_normal cursor_invisible cursor_visible)
cursor_on=^X
cursor_off=^W
cursor_on_mode_bit-00800000
### ?? parm_mode_val-CURSORON-00000000=?25
### NO cursor_type=
### NO cursor_type_turns_cursor_on
##--- KEYPAD (keypad_xmit keypad_local ) ---##
### NO enter_appl_keypad_mode=
### NO exit_appl_keypad_mode=
appl_keypad_mode_on_mode_bit-00400000
### ?? parm_mode_val-APPKEYON-00000000=?4
### NO enter_cursor_key_mode=
### NO exit_cursor_key_mode=
cursor_key_mode_on_mode_bit-00200000
### ?? parm_mode_val-CURKEYON-00000000=?4
##--- TABS ---##
### back_tab=
### clear_all_tabs=
### set_tab=
##--- FUNCTION KEYS ---##
### ?? function_key_clear=
### ?? function_key_clear_mode_yes=number
### ?? function_key_clear_mode_no=number
### ?? parm_pkey_key=
### ?? parm_pkey_key=
### function key load - may have imbedded ^C if preceded by ^P
### pass=\EG%s1^C   book says G is g
pkey_key=\Eg%p1%c2;%s1^C
### NO ## TBD pkey_local=
### NO ## TBD pkey_xmit=
### ?? function_key_string_is_hex
### ?? function_key-id-delim=
### ?? shift_function_key
### ?? function_key-id-delim=
function_key-!=^B0\n
function_key-"=^B1\n
function_key-#=^B2\n
function_key-$=^B3\n
function_key-%=^B4\n
function_key-&=^B5\n
function_key-'=^B6\n
function_key-(=^B7\n
function_key-)=^B8\n
function_key-*=^B9\n
function_key-+=
function_key-,=
shift_function_key
function_key-45=
function_key-.=
function_key-/=
function_key-0=
function_key-1=
function_key-2=
function_key-3=
function_key-4=
function_key-5=
function_key-6=
function_key-7=
function_key-8=
##--- CURSOR KEYS ---##
key_home=^A
key_up=^Z
### key_down=
key_right=^F
key_left=^U
### key_backspace=
##--- PRINTER ---##
### NO ## TBD print_screen=\Ex
### NO ## TBD prtr_non=
### NONTRANSPARENT - on=^T$<?/>  off=^R$<?/>
### TRANSPARENT - needs "short" delay
### TBD prtr_off=\E4$<100/>
### TBD prtr_on=\E3$<100/>
##--- STATUS LINE ---##
### should default to on so backwards disable_status_user=/EB enable=\Eb
status_on=\EB
status_off=\Eb
### display_terminal_status=\EN display_user_or_message=\En
### ?? status_on=\EN
### ?? status_off=\En
### ?? status_off_clears_status_type
### ?? status_off_clears_status_line
### ?? status_type_turns_status_on
### ?? status_type=
### ?? status_type=
### ?? status_type_ruins_status_line
### NO ## COMBINE to_status_line=
### NO ## COMBINE from_status_line=
### FROM_COMBINATION status_line=\Ec%s1^Z
### ?? status_line_turns_status_on
##--- MISC ---##
### ASSUMED bell=^G
### ONLY IF PADDING carriage_return=\r
### NO ## ONLY IF REAL new_line=
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
### NO ## NOT_USED key_clear=
### NO ## NOT_USED key_ctab=
### NO ## NOT_USED key_dc=
### NO ## NOT_USED key_dl=
### NO ## NOT_USED key_eic=
### NO ## NOT_USED key_eol=
### NO ## NOT_USED key_eos=
### NO ## NOT_USED key_ic=
### NO ## NOT_USED key_il=
### NO ## NOT_USED key_ll=
### NO ## NOT_USED key_npage=
### NO ## NOT_USED key_ppage=
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
### NOTE 2page mode = \E;  page 1 =\ES   page 2 = \Es
pages=2
### ?? display_page_number=
### ?? parm_display_page_next=
### ?? parm_display_page_prev=
display_page_next=\Es
display_page_prev=\ES
##--- INIT ---##
### 2page mode = \E;
enter_ca_mode=\E0P^X^L\E;
exit_ca_mode=^O^X\E0@
### NO ## SPLITUP init_1string=
### NO ## SPLITUP init_2string=
### NO ## SPLITUP init_3string=
### NO ## SPLITUP init_file=
### NO ## SPLITUP init_prog=
### NO ## SPLITUP reset_1string=
### NO ## SPLITUP reset_2string=
### NO ## SPLITUP reset_3string=
### NO ## SPLITUP reset_file=
##--- MODE ---##
### NO ## SPLITUP flash_screen=
### ?? mode-00000000-00000000=
### ?? ignore=
### ?? pass=
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
### keyboard lock=\E5 lock=^D unlock=\E6 unlock=^B
ignore-keyboard_lock=\E5
ignore-keyboard_lock=^D
pass-keyboard_unlock=\E6
pass-keyboard_unlock=^B
### SYSTEM DEPENDENT ###
### VERSION3 ###
use=ncr7930.fm
