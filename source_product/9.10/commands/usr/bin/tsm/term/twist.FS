# FacetTerm  twist   description file 09/29/89
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: twist.FS,v 70.1 92/03/09 16:05:47 ssa Exp $
columns=80
lines=24
### NO graphics_8_bit
### terminal assumed to be in xon-xoff mode
xon_xoff
##--- MARGINS ---##
### NO auto_left_margin
### ASSUMED auto_right_margin
eat_newline_glitch
auto_wrap_on=\E[?7h
auto_wrap_off=\E[?7l
##--- CLEAR ---##
### SEPARATED clear_screen=\E[H\E[2J
clear_screen=\E[0X
clear_screen=\E[X
clear_all=\E[2J
### NO ?? clear_screen_mode_off--P
clr_eol=\E[0K
clr_eol=\E[K
### NO clr_eol_w_attr=
clr_eos=\E[0J
clr_eos=\E[J
### NO clr_eos_w_attr=
##--- CURSOR ---##
cursor_address=\E[%i%p1%dH
cursor_address=\E[%i%p1%d;H
cursor_address=\E[%i;%p2%dH
cursor_address=\E[%i%p1%d;%p2%df
cursor_address=\E[%i%p1%d;%p2%dH
decode_type-0123456789;?=\E[
cursor_home=\E[f
cursor_home=\E[;f
cursor_home=\E[;H
cursor_home=\E[H
cursor_up=\E[A
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
set_margins=\E[%i%p1%d;%p2%dv
### ignore=\E[1;80v
### ignore=\E[30;80v
##--- INS/DEL ---##
insert_line=\E[L
### NO insdel_line_move_col0
parm_insert_line=\E[%p1%dL
delete_line=\E[M
parm_delete_line=\E[%p1%dM
### NO insert_character=
### NO insert_padding=
### NO ## NOT_IMP parm_ich=
enter_insert_mode=\E[4h
exit_insert_mode=\E[4l
delete_character=\E[P
parm_delete_character=\E[%p1%dP
### NO ## NOT_IMP erase_chars=
### NO ## NOT_IMP enter_delete_mode=
### NO ## NOT_IMP exit_delete_mode=
### NO ## NOT_IMP repeat_char=
##--- ATTRIBUTES URBDOIPA ---##
##---                 x   ---##
### NOTE magic_cookie_glitch=0
### USE_PARM attribute-U=\E[4m
### USE_PARM attribute-R=\E[7m
### USE_PARM attribute-B=\E[5m
### NO USED_IN_CHARSET attribute-D=
### USE_PARM attribute-O=\E[1m
### NOT_IN_MANUAL attribute-I=\E8m
### Start security field.
attribute-I-=\E[1s
attribute--I=\E[0s
attribute--I=\E[s
### NO USED_IN_CHARSET attribute-P=
### USE_CHARSET attribute-A=^N
### USE_CHARSET attribute--A=^O
### NO attribute_next-A-=
### SPLITUP set_attributes=\E[0%?%p1%p6%|%t;1%;%?%p2%t;4%;
###                        %?%p1%p3%|%t;7%;%?%p4%t;5%;m%?%p9%t^N%e^O%;
##--- MULTIPLE ATTRIBUTES URBDOIPA ---##
##---                     475 1    ---##
parm_attribute--=\E[%7MMm
parm_attribute--URBO-=\E[;%7MMm
parm_attribute_out=\E[%s1m
parm_attribute_out=\E[%s1;%s2m
parm_attribute_out=\E[%s1;%s2;%s3m
parm_attribute_out=\E[%s1;%s2;%s3;%s4m
parm_attribute_out=\E[%s1;%s2;%s3;%s4;%s5m
parm_attribute_out=\E[%s1;%s2;%s3;%s4;%s5;%s6m
parm_attribute_out=\E[%s1;%s2;%s3;%s4;%s5;%s6;%s7m
parm_attribute_out=\E[%s1;%s2;%s3;%s4;%s5;%s6;%s7;%s8m
parm_attribute_val--URBO=0
parm_attribute_val-U-=4
parm_attribute_val-R-=7
parm_attribute_val-B-=5
parm_attribute_val-O-=1
exit_attribute_mode--URBO=\E[0m
exit_attribute_mode--URBO=\E[m
##--- MAGIC COOKIE ATTRIBUTES ---##
### NO magic=
### NO nomagic=
### NO flash_screen=
##--- CHARACTER SET ATTRIBUTES ---##
lock_shift_0=^O
select_character_set_0--=\E(B
select_character_set_0-P=\E(A
select_character_set_0-A=\E(0
select_character_set_0-PA=\E(1
select_character_set_0-D=\E(2
lock_shift_1=^N
select_character_set_1--=\E)B
select_character_set_1-P=\E)A
select_character_set_1-A=\E)0
select_character_set_1-PA=\E)1
select_character_set_1-D=\E)2
###single_shift_2=\EN
###select_character_set_2-A-=
###single_shift_3=\EO
###select_character_set_3-A-=
##--- LINE ATTRIBUTES ---##
line_attribute_off=\E#5
line_attribute=\E#6
line_attribute=\E#3
line_attribute=\E#4
##--- CURSOR (cursor_normal cursor_invisible cursor_visible)
cursor_on=\E[?35l
cursor_off=\E[?35h
### NO cursor_type=
### NO cursor_type_turns_cursor_on
##--- KEYPAD (keypad_xmit keypad_local ) ---##
enter_appl_keypad_mode=\E=
exit_appl_keypad_mode=\E>
enter_cursor_key_mode=\E[?1h
exit_cursor_key_mode=\E[?1l
##--- TABS ---##
### TBD init_tabs=8
### NO ## ASSUMED tab=
### back_tab=
### NOT_IMP ??clear_tab=\E[g
### NOT_IMP ??clear_tab=\E[0g
clear_all_tabs=\E[3g
set_tab=\EH
##--- FUNCTION KEYS ---##
function_key_clear=\E\17751 00:CLEAR
function_key_clear=\E\17751 0:CLEAR
pkey_key=\E\17751 %p1%d:%63S1
function_key-01-"=\E1
function_key-02-"=\E2
function_key-03-"=\E3
function_key-04-"=\E4
function_key-05-"=\E5
function_key-06-"=\E6
function_key-07-"=\E7
function_key-08-"=\E8
function_key-09-"=\E9
function_key-10-"=\E:
function_key-11-"=\E;
function_key-12-"=\E<
shift_function_key
function_key-13-"=\Ea
function_key-14-"=\Eb
function_key-15-"=\Ec
function_key-16-"=\Ed
function_key-17-"=\Ee
function_key-18-"=\Ef
function_key-19-"=\Eg
function_key-20-"=\Eh
function_key-21-"=\Ei
function_key-22-"=\Ej
function_key-23-"=\Ek
function_key-24-"=\El
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
### TBD key_f1=\EOq
### TBD key_f2=\EOr
### TBD key_f3=\EOs
### TBD key_f4=\EOt
### TBD key_f5=\EOu
### TBD key_f6=\EOv
### TBD key_f7=\EOw
### TBD key_f8=\EOx
### TBD key_f9=\EOy
##--- PRINTER ---##
### NO ## TBD print_screen=
### NO ## TBD prtr_non=
### NO ## TBD prtr_off=
### NO ## TBD prtr_on=
transparent_print_on=\E[5i
transparent_print_off=\E[4i
##--- STATUS LINE ---##
status_line=\E[?L%s1\r
status_line_turns_status_on
##--- MISC ---##
### ASSUMED bell=^G
### ONLY IF PADDING carriage_return=\r
### NOTE move_insert_mode
### NOTE move_standout_mode
### NOTE virtual_terminal=3
### NOTE key_a1=\EOq
### NOTE key_a3=\EOs
### NOTE key_b2=\EOr
### NOTE key_c1=\EOp
### NOTE key_c3=\EOn
##--- MULTIPAGE ---##
pages=3
### display_page_number=
parm_display_page_next=\E[%p1%dU
parm_display_page_prev=\E[%p1%dV
### display_page_next=
### display_page_prev=
##--- INIT ---##
enter_ca_mode=\E[?31l
### exit_ca_mode=
### NOTE reset_2string=\E>\E[?3l\E[?4l\E[?5l\E[?7h\E[?8h
### screen mode h=reverse l=normal
### mode-00000000-00000001=\E[?5l
### mode-00000001-00000000=\E[?5h
ignore=\E[?5l
ignore=\E[?5h
### smooth scroll h=on l=off
mode-00000000-00000002=\E[?4l
mode-00000002-00000000=\E[?4h
### NO ignore=
### auto repeat h=on l=off
pass=\E[?8h
pass=\E[?8l
### columns h=132 l=80
ignore=\E[?3l
### origin mode l=full screen h=in scroll region
ignore=\E[?6l
### newline mode h=lf and cr l=lf
pass=\E[20l
### keyboard lock h=on l=off
ignore=\E2h
pass=\E2l
### local echo h=on l=off
pass=\E12l
### editing mode h=on l=off
pass=\E[?10l
### edit key execution h=immediate l=deferred
pass=\E[?16l
### protected field attributes 0=none 1=bold 2=under 3=blink 7=reverse 254=all
pass=\E[0}
pass=\E[?%p1%d;%p2%dT
pass=\E[?40l
pass=\E[?41l
pass=\E[?42l
pass=\E[?43l
pass=\E[?44l
pass=\E[?45l
pass=\E[?46l
pass=\E[?40h
pass=\E[?41h
pass=\E[?42h
pass=\E[?43h
pass=\E[?44h
pass=\E[?45h
pass=\E[?46h
pass=\E[?47h
pass=\E[?48h
pass=\E[?49h
## led_control=\E[%p1%dq
mode-00000010-00000000=\E[1q
mode-00000020-00000000=\E[2q
mode-00000040-00000000=\E[3q
mode-00000080-00000000=\E[4q
multi_mode-000000F0-00000000=\E[0q
multi_mode-000000F0-00000010=\E[0;1q
multi_mode-000000F0-00000020=\E[0;2q
multi_mode-000000F0-00000030=\E[0;1;2q
multi_mode-000000F0-00000040=\E[0;3q
multi_mode-000000F0-00000050=\E[0;1;3q
multi_mode-000000F0-00000060=\E[0;2;3q
multi_mode-000000F0-00000070=\E[0;1;2;3q
multi_mode-000000F0-00000080=\E[0;4q
multi_mode-000000F0-00000090=\E[0;1;4q
multi_mode-000000F0-000000A0=\E[0;2;4q
multi_mode-000000F0-000000B0=\E[0;1;2;4q
multi_mode-000000F0-000000C0=\E[0;3;4q
multi_mode-000000F0-000000D0=\E[0;1;3;4q
multi_mode-000000F0-000000E0=\E[0;2;3;4q
multi_mode-000000F0-000000F0=\E[0;1;2;3;4q
### Enter ansi mode
ignore=\E<
## Noexistent code, should probably be \E[?1l. (Qoffice init string)
ignore=\E[1l
## Report Display mode  (KIS text)
pass=\E[?1n
paste_eol-UTS=\EOS\377
paste_eol-UPX=\040
ignore=\177
use=twist.fm
