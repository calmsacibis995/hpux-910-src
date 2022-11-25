# FacetTerm  k2|kt22ix|kimtron kt-22/pc ix  description file 09/29/89
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: kt22ix.FS,v 70.1 92/03/09 16:01:42 ssa Exp $
columns=80
### ?? columns_wide=number
### ?? columns_wide_on=
### ?? columns_wide_off=
### ?? columns_wide_clears_screen
columns_wide_mode_on_mode_bit-08000000
### ?? parm_mode_val-COLSWIDE-00000000=?3
lines=25
graphics_8_bit
### ?? graphics_escape_control
### ?? input_8_bit
xon_xoff
##--- MARGINS ---##
### NO auto_left_margin
### ASSUMED auto_right_margin
eat_newline_glitch
### auto_wrap_on and auto_repeat_mode from sco init
auto_wrap_on=\E[?7h
auto_wrap_off=\E[?7l
auto_wrap_mode_bit-04000000
parm_mode_val-AUTOWRAP-00000000=?7
##--- CLEAR ---##
clear_screen=\E[2J
### ?? clear_all=\E[2J
clr_eol=\E[0K
clr_eol=\E[K
### out_clr_eol=\E[K
### NO clr_eol_w_attr=\E[K
### NO ## clr_eol_chars=
### NO ## set_attr_eol=
clr_eos=\E[0J
clr_eos=\E[J
### out_clr_eos=\E[J
### NO clr_eos_w_attr=\E[J
### NO ## clr_eos_chars=
### NO ## set_attr_eos=
##--- CURSOR ---##
cursor_address=\E[%i%p1%dH
cursor_address=\E[%i%p1%d;H
cursor_address=\E[%i;%p2%dH
cursor_address=\E[%i%p1%d;%p2%df
cursor_address=\E[%i%p1%d;%p2%dH
### ?? cursor_address_wide=
decode_type-0123456789;?=\E[
cursor_home=\E[f
cursor_home=\E[;f
cursor_home=\E[;H
cursor_home=\E[H
cursor_up=\E[A
### ?? cursor_up_at_home_wraps_ll
parm_up_cursor=\E[%p1%dA
cursor_down=\E[B
parm_down_cursor=\E[%p1%dB
cursor_right=\E[C
parm_right_cursor=\E[%p1%dC
cursor_left=\E[D
parm_left_cursor=\E[%p1%dD
### NO ## NOT_IMP column_address=
### NO ## NOT_IMP row_address=
### NO ## NOT_IMP cursor_mem_address=
### NO ## NOT_IMP cursor_to_ll=
##--- SCROLL ---##
change_scroll_region=\E[r
change_scroll_region=\E[%i%p1%dr
change_scroll_region=\E[%i%p1%d;r
change_scroll_region=\E[%i;%p2%dr
change_scroll_region=\E[%i%p1%d;%p2%dr
scroll_forward=\ED
scroll_could_be_cursor_only
### NO ## NOT_IMP parm_index=
scroll_reverse=\EM
### NO ## NOT_IMP parm_rindex=
save_cursor=\E7
restore_cursor=\E8
##--- INS/DEL ---##
insert_line=\E[L
### insdel different from vt220
insdel_line_move_col0
parm_insert_line=\E[%p1%dL
delete_line=\E[M
parm_delete_line=\E[%p1%dM
insert_character=\E[1@
insert_character=\E[@
### NO insert_padding=
### NOT_IMP parm_ich=\E[%p1%d@
enter_insert_mode=\E[4h
exit_insert_mode=\E[4l
insert_mode_on_mode_bit-02000000
parm_mode_val-INSERTON-00000000=4
delete_character=\E[P
parm_delete_character=\E[%p1%dP
### NOT_IMP erase_chars=\E[%p1%dX
### NO ## NOT_IMP enter_delete_mode=
### NO ## NOT_IMP exit_delete_mode=
### NO ## NOT_IMP repeat_char=
##--- ATTRIBUTES URBDOIPA ---##
### NO ## NOT_IMP set_attributes=
### NO ## NOTE magic_cookie_glitch=number
### USE_REVERSE attribute-?=\E[7m
### USE_REVERSE attribute--?=\E[27m
### USE_PARM attribute-U=\E[4m
### USE_PARM attribute--U=\E[24m
### USE_PARM attribute-R=\E[7m
### USE_PARM attribute-B=\E[5m
### USE_PARM attribute-D=\E[2m
### USE_PARM attribute-O=\E[1m
### USE_PARM attribute-I=\E[8m
### USED_IN_CHARSET attribute-P=
### USE_CHARSET attribute-A=\E(s
### USE_CHARSET attribute-A=\E(0
### USE_CHARSET attribute--A=\E(B
### NO attribute_next-A-=
### USE_PARM attribute--ODI=\E[22m
### USE_PARM attribute--ODI=\E[23m
### USE_PARM attribute--B=\E[25m
### USE_PARM attribute--R=\E[27m
### USE_PARM attribute--ODI=\E[28m
### ODUBRI
### USE_PARM attribute-OU-=\E[1;4m
### USE_PARM attribute-OB-=\E[1;5m
### USE_PARM attribute-OR-=\E[1;7m
### USE_PARM attribute-DU-=\E[2;4m
### USE_PARM attribute-DB-=\E[2;5m
### USE_PARM attribute-DR-=\E[2;7m
### USE_PARM attribute-UB-=\E[4;5m
### USE_PARM attribute-UR-=\E[4;7m
### USE_PARM attribute-BR-=\E[5;7m
### TOO_MANY attribute-O-DUBRI=\E[0;1m
### TOO_MANY attribute-D-OUBRI=\E[0;2m
### TOO_MANY attribute-U-ODBRI=\E[0;4m
### TOO_MANY attribute-B-ODURI=\E[0;5m
### TOO_MANY attribute-R-ODUBI=\E[0;7m
### TOO_MANY attribute-I-ODUBR=\E[0;8m
### USE_PARM attribute-OU-DBRI=\E[0;1;4m
### USE_PARM attribute-OB-DURI=\E[0;1;5m
### USE_PARM attribute-OR-DUBI=\E[0;1;7m
### USE_PARM attribute-DU-OBRI=\E[0;2;4m
### USE_PARM attribute-DB-OURI=\E[0;2;5m
### USE_PARM attribute-DR-OUBI=\E[0;2;7m
### USE_PARM attribute-UB-ODRI=\E[0;4;5m
### USE_PARM attribute-UR-ODBI=\E[0;4;7m
### USE_PARM attribute-BR-ODUI=\E[0;5;7m
### USE_PARM exit_attribute_mode--ODUBRI=\E[0m
### USE_PARM exit_attribute_mode--ODUBRI=\E[m
##--- MULTIPLE ATTRIBUTES URBDOIPA ---##
##---                     475218   ---##
parm_attribute=\E[%7MMm
parm_attribute--URBDOI-=\E[;%7MMm
parm_attribute_out=\E[%s1m
parm_attribute_out=\E[%s1;%s2m
parm_attribute_out=\E[%s1;%s2;%s3m
parm_attribute_out=\E[%s1;%s2;%s3;%s4m
parm_attribute_out=\E[%s1;%s2;%s3;%s4;%s5m
parm_attribute_out=\E[%s1;%s2;%s3;%s4;%s5;%s6m
parm_attribute_out=\E[%s1;%s2;%s3;%s4;%s5;%s6;%s7m
parm_attribute_out=\E[%s1;%s2;%s3;%s4;%s5;%s6;%s7;%s8m
parm_attribute_val--URBDOI=0
parm_attribute_val-U-=4
parm_attribute_val-R-=7
parm_attribute_val-B-=5
parm_attribute_val-D-O=2
parm_attribute_val-O-D=1
parm_attribute_val-I-=8
parm_attribute_val-I-=8
parm_attribute_val--ODI=22
parm_attribute_val--ODI=23
parm_attribute_val--U=24
parm_attribute_val--B=25
parm_attribute_val--R=27
parm_attribute_val--ODI=28
### ?? parm_attribute_val-P-=number
### ?? parm_attribute_val-A-=number
exit_attribute_mode--URBDOI=\E[m
##--- MAGIC COOKIE ATTRIBUTES ---##
### USE_ATTR magic-standout=\E[7m
### USE_ATTR magic-underline=\E[4m
### NO ## USE_ATTR magic-reverse=
### NO ## USE_ATTR magic-blink=
### NO ## USE_ATTR magic-dim=
### NO ## USE_ATTR magic-bold=
### NO ## USE_ATTR magic-secure=
### NO ## USE_ATTR magic-protected=
### USE_CHARSET magic-alt_charset=\E(s
### USE_CHARSET magic-alt_charset=\E(0
### NO ## USE_ATTR nomagic-exit_attribute_mode=
### USE_CHARSET nomagic-exit_standout_mode=\E[27m
### USE_ATTR nomagic-exit_underline_mode=\E[24m
### USE_CHARSET nomagic-exit_alt_charset_mode=\E(B
##--- CHARACTER SET ATTRIBUTES URBDOIPA ---##
### ?? lock_shift_0=
select_character_set_0--=\E(B
select_character_set_0-A-=\E(s
select_character_set_0-P-=\E(0
### ?? lock_shift_1=
### ?? select_character_set_1--=
### ?? select_character_set_1--=
### ?? single_shift_2=
### ?? select_character_set_2--=
### ?? single_shift_3=
### ?? select_character_set_3--=
##--- LINE ATTRIBUTES ---##
### ?? line_attribute_off=
### ?? line_attribute=
### ?? line_attribute=
##--- CURSOR (cursor_normal cursor_invisible cursor_visible)
cursor_on=\E[?25h
cursor_off=\E[?25l
cursor_on_mode_bit-00800000
parm_mode_val-CURSORON-00000000=?25
### NO cursor_type=
### NO cursor_type_turns_cursor_on
##--- KEYPAD (keypad_xmit keypad_local ) ---##
enter_appl_keypad_mode=\E=
exit_appl_keypad_mode=\E>
appl_keypad_mode_on_mode_bit-00400000
### ?? parm_mode_val-APPKEYON-00000000=
enter_cursor_key_mode=\E[?1h
exit_cursor_key_mode=\E[?1l
cursor_key_mode_on_mode_bit-00200000
parm_mode_val-CURKEYON-00000000=?1
##--- TABS ---##
### NOTE init_tabs=8
### NO ## ASSUMED tab=
### TBD back_tab=\EI
clear_all_tabs=\E[3g
set_tab=\EH
##--- FUNCTION KEYS ---##
### ?? function_key_clear=
### ?? function_key_clear_mode_yes=number
### ?? function_key_clear_mode_no=number
### ?? parm_pkey_key=
### ?? parm_pkey_key=
### NO pkey_key=
### ?? function_key_string_is_hex
### ?? function_key-id-delim=
### ?? shift_function_key
### ?? function_key-id-delim=
### NO ## BY_HAND function_key-key_f0=\EOP
### NO ## BY_HAND function_key-key_f1=\EOQ
### NO ## BY_HAND function_key-key_f2=\EOR
### NO ## BY_HAND function_key-key_f3=\EOS
### NO ## BY_HAND function_key-key_f4=\EOT
### NO ## BY_HAND function_key-key_f5=\E[17~
### NO ## BY_HAND function_key-key_f6=\E[18~
### NO ## BY_HAND function_key-key_f7=\E[19~
### NO ## BY_HAND function_key-key_f8=\E[20~
### NO ## BY_HAND function_key-key_f9=\E[21~
### NO ## BY_HAND function_key-key_f10=
##--- CURSOR KEYS ---##
### NO key_home=
key_up=\E[A
key_up=\EOA
key_down=\E[B
key_down=\EOB
key_left=\E[D
key_left=\EOD
key_right=\E[C
key_right=\EOC
### SAME_AS_BACKSPACE key_backspace=^H
##--- PRINTER ---##
##--- STATUS LINE ---##
### using 25th line as text
### ?? status_on=
### ?? status_off=
### ?? status_off_clears_status_type
### ?? status_off_clears_status_line
### ?? status_type_turns_status_on
### ?? status_type=
### ?? status_type=
### ?? status_type_ruins_status_line
### NO ## COMBINE to_status_line=
### NO ## COMBINE from_status_line=
### FROM_COMBINATION status_line=
### ?? status_line_turns_status_on
##--- MISC ---##
### ASSUMED bell=^G
### ONLY IF PADDING carriage_return=\r
new_line=\EE
### NO ## NOT_IMP form_feed=
### NO ## OK_IF_MAGIC transparent_underline
### NO ## NOT_IMP ceol_standout_glitch
### NO ## NOT_IMP erase_overstrike
### NO ## NOT_IMP generic_type
### NO ## NOT_IMP hard_copy
### NO ## NOT_IMP has_meta_key
### NO ## NOT_IMP insert_null_glitch
### NOT_IMP memory_above
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
### NOT_USED lab_f0=\E[23~
### NOT_USED lab_f1=\E[24~
### NOT_USED lab_f2=\E[25~
### NOT_USED lab_f3=\E[26~
### NOT_USED lab_f4=\E[28~
### NOT_USED lab_f5=\E[29~
### NOT_USED lab_f6=\E[31~
### NOT_USED lab_f7=\E[32~
### NOT_USED lab_f8=\E[33~
### NOT_USED lab_f9=\E[34~
### NO ## NOT_USED lab_f10=
##--- MULTIPAGE ---##
pages=3
### ?? display_page_number=
parm_display_page_next=\E[%p1%dU
parm_display_page_prev=\E[%p1%dV
### ?? display_page_next=
### ?? display_page_prev=
##--- INIT ---##
### NOTE init_file=/usr/lib/tabset/kt22ix
###              clear_tabs set_tab  25_line_page 80_col 25t  soft_reset
### CONTENTS_ARE \E[3g      \EH      \E[1;1z      \E[?3l \E[z \E[!p
###              jump_scroll dark_screen page_mode normal_UDK  auto_wrap & rep
### CONTENTS ARE \E[?4;5;28;30l                                \E[?7;8h
enter_ca_mode=\E[1;1z\E[?3l\E[z\E[!p\E[?4l\E[?5l\E[?28l\E[?30l\E[?7h\E[?8h
### NO exit_ca_mode=
### page_length \E[1;%p1%dz 24=0  25=1  50=2  75=3 - set to 25    enter_ca_mode
ignore=\E[1;1z
### 25th_line  \E[0;%p1%d  text=0  status=1  user=2  set text     enter_ca_mode
ignore=\E[z
### soft_reset \E[!p                                              enter_ca_mode
ignore=\E[!p
### keyboard_action_mode	n-lock=\E[2h	p-unlock=\E[2l
parm_mode_val-NOTIMP..-PASS....=2
### local_echo			p-off=\E[12h	n-on=\E[12l
parm_mode_val-PASS....-NOTIMP..=12
### new_line_mode		n-on=\E[20h	p-off=\E[20l
parm_mode_val-NOTIMP..-PASS....=20
### columns   			n-132=\E[?3h	i-80=\E[?3l	  enter_ca_mode
parm_mode_val-NOTIMP..-IGNORE..=?3
### smooth_scroll		on=\E[?4h	*off=\E[?4l   	  enter_ca_mode
mode-00000000-00000001=\E[?4l
mode-00000001-00000000=\E[?4h
parm_mode_val-00000001-00000000=?4
### screen_mode			reverse=\E[?5h	*normal=\E[?5l	  enter_ca_mode
mode-00000000-00000002=\E[?5l$<200/>
mode-00000002-00000000=\E[?5h$<200/>
parm_mode_val-00000002-00000000=?5
### origin_mode			n-rel=\E[?6h	p-absolute=\E[?6l
parm_mode_val-NOTIMP..-IGNORE..=?6
### auto_repeat_mode		p-enable=\E[?8h	p-disable=\E[?8l  enter_ca_mode
parm_mode_val-PASS....-PASS....=?8
### print_form_feed_mode	yes=\E[?18h	no=\E[?18l
### window_mode			n-window=\E[?28h i-page=\E[?28l   enter_ca_mode
parm_mode_val-NOTIMP..-IGNORE..=?28
### user_define_key_mode	n-extend=\E[?30h i-normal=\E[?30l enter_ca_mode
parm_mode_val-NOTIMP..-IGNORE..=?30
##--- MODE ---##
### NO ## SPLITUP flash_screen=
### ?? mode-00000000-00000000=
### ?? ignore=
### ?? pass=
##--- PARM_MODE ---##
parm_mode_on=\E[%10M?h
parm_mode_off=\E[%10M?l
parm_mode_private_propogates
ignore_mode_bit-80000000
### ?? parm_mode_val-80000000-IGNORE..=?99
pass_mode_bit-40000000
### ?? parm_mode_val-40000000-PASS....=?99
notimp_mode_bit-20000000
### ?? parm_mode_val-20000000-NOTIMP..=?99
### NCR TOWER 32 ###
### NOTE horiz_line=q
### NOTE vert_line=x
### NOTE up_left_corner=l
### NOTE up_right_corner=k
### NOTE low_left_corner=m
### NOTE low_right_corner=j
### NOTE dn_t_bar=w
### NOTE lt_t_bar=t
### NOTE rt_t_bar=u
### NOTE up_t_bar=v
### NOTE t_cross=n
use=kt22ix.fm
