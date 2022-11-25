# FacetTerm  vt240  (80 column only)  description file  06/13/91
#    Graphics-pass-through set for 20/20.
#    See also Vt240 for 80/132 column.
#
#    !!! IMPORTANT NOTE !!!
#    Set Terminal to: XON/XOFF
#
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: vt240.FS,v 70.1 92/03/09 16:08:27 ssa Exp $
xon_xoff
columns=80
### NOT_IN_80 columns_wide=132
### NOT_IN_80 columns_wide_on=\E[?3h
### NOT_IN_80 columns_wide_off=\E[?3l
pass=\E[?3l
parm_mode_val-NOTIMP..-PASS....=?3
### NOT_IN_80 columns_wide_clears_screen
lines=24
pass-7bitmode=\E F
##--- INIT ---##
pass=\E[62;1"p
### use=vt240graph.fc
## FacetTerm vt240graph  description file 6/20/88
## 20/20 graphics
graph_mode_clear=\EPps(a[0,479][799,0])w(i2v)s(e)t(s1)s(i0)\E\\
graph_mode_clear=\EPpp[0,0]w(i0p1s[,420])v[799]w(i2p1s0)w(i2p1)\E\\
graph_mode_start=\EPp%s1\E\\
use=vt220ans.fc
### use=vt220com.fc
## FacetTerm  vt220com   description file  09/06/88
### NOT_IN_COMMON xon_xoff
### NOT_IN_COMMON columns=80
### NOT_IN_COMMON columns_wide=132
### NOT_IN_COMMON columns_wide_on=\E[?3h
### NOT_IN_COMMON columns_wide_off=\E[?3l
### NOT_IN_COMMON parm_mode_val-COLSWIDE-00000000=?3
### NOT_IN_COMMON columns_wide_clears_screen
### NOT_IN_COMMON lines=24
### NO graphics_8_bit
##--- MARGINS ---##
### NO auto_left_margin
### ASSUMED auto_right_margin
eat_newline_glitch
auto_wrap_on=\E[?7h
auto_wrap_off=\E[?7l
##--- CLEAR ---##
### SEPARATED clear_screen=\E[H\E[2J
clear_all=\E[2J
clr_eol=\E[0K
clr_eol=\E[K
### NO clr_eol_w_attr=\E[K
clr_eos=\E[0J
clr_eos=\E[J
### NO clr_eos_w_attr=\E[J
##--- CURSOR ---##
cursor_address=\E[%i%p1%dH
cursor_address=\E[%i%p1%d;H
cursor_address=\E[%i;%p2%dH
cursor_address=\E[%i%p1%d;%p2%df
cursor_address=\E[%i%p1%d;%p2%dH
### cursor_report
answer-..cursor_address=\E[%i%p1%d;%p2%dR
question-cursor_address=\E[6n
decode_type-0123456789;?=\E[
cursor_home=\E[;f
cursor_home=\E[f
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
##--- INS/DEL ---##
insert_line=\E[L
### NO insdel_line_move_col0
parm_insert_line=\E[%p1%dL
delete_line=\E[M
parm_delete_line=\E[%p1%dM
insert_character=\E[@
insert_character=\E[1@
### NO insert_padding=
### NOT_IMP parm_ich=\E[%p1%d@
enter_insert_mode=\E[4h
exit_insert_mode=\E[4l
delete_character=\E[P
parm_delete_character=\E[%p1%dP
### NOT_IMP erase_chars=\E[%p1%dX
### NO ## NOT_IMP enter_delete_mode=
### NO ## NOT_IMP exit_delete_mode=
### NO ## NOT_IMP repeat_char=
##--- ATTRIBUTES URBDOIPA ---##
### NOTE magic_cookie_glitch=0
### standout
### USE_MULTIPLE attribute-OR=\E[1;7m
### USE_PARM attribute-U=\E[4m
### USE_PARM attribute--U=\E[24m
### USE_PARM attribute-R=\E[7m
### USE_PARM attribute--R=\E[27m
### USE_PARM attribute-B=\E[5m
### USE_PARM attribute--B=\E[25m
### USE_PARM attribute-O-D=\E[1m
### USE_PARM attribute-D-O=\E[2m
### USE_PARM attribute--OD=\E[22m
### NOT_IN_MANUAL USED_IN_CHARSET attribute-I=\E8m
### USED_IN_CHARSET attribute-P=
### SAME_AS_EXIT attribute--?=\E[m
### ?? attribute-A=
### ?? attribute--A=
### ?? attribute_next-A-=
##--- MULTIPLE ATTRIBUTES URBDOIPA ---##
##---                     47521    ---##
parm_attribute=\E[%7MMm
parm_attribute--URBDO-=\E[;%7MMm
parm_attribute_out--=\E[%s1m
parm_attribute_out--=\E[%s1;%s2m
parm_attribute_out--=\E[%s1;%s2;%s3m
parm_attribute_out--=\E[%s1;%s2;%s3;%s4m
parm_attribute_out--=\E[%s1;%s2;%s3;%s4;%s5m
parm_attribute_out--=\E[%s1;%s2;%s3;%s4;%s5;%s6m
parm_attribute_out--=\E[%s1;%s2;%s3;%s4;%s5;%s6;%s7m
parm_attribute_out--=\E[%s1;%s2;%s3;%s4;%s5;%s6;%s7;%s8m
parm_attribute_val--URBDO=0
parm_attribute_val-U-=4
parm_attribute_val-R-=7
parm_attribute_val-B-=5
parm_attribute_val-D-O=2
parm_attribute_val-O-D=1
parm_attribute_val--DO=22
parm_attribute_val--U=24
parm_attribute_val--B=25
parm_attribute_val--R=27
exit_attribute_mode--URBDO=\E[0m
exit_attribute_mode--URBDO=\E[m
##--- MAGIC COOKIE ATTRIBUTES ---##
### alt_charset
### USE_ATTR magic-blink=\E[5m
### bold
### dim
### USE_ATTR magic-normal=\E[22m
### protected
### USE_ATTR magic-reverse=\E[7m
### secure
### USE_ATTR magic-standout=\E[1;7m
### USE_ATTR magic-underline=\E[4m
### USE_ATTR nomagic-exit_attribute_mode=\E[m
### exit_alt_charset_mode
### USE_ATTR nomagic-exit_standout_mode=\E[m
### exit_underline_mode
### USE_ATTR nomagic-exit_underline_mode=\E[m
##--- CHARACTER SET ATTRIBUTES URBDOIPA ---##
##---                               XXX ---##
lock_shift_0=^O
select_character_set_0--=\E(B
select_character_set_0-A=\E(0
select_character_set_0-P=\E(<
lock_shift_1=^N
select_character_set_1--=\E)B
select_character_set_1-A=\E)0
select_character_set_1-P=\E)<
select_character_set_1-AP=\E) @
lock_shift_2=\En
single_shift_2=\EN
select_character_set_2--=\E*B
select_character_set_2-A=\E*0
select_character_set_2-P=\E*<
lock_shift_3=\Eo
single_shift_3=\EO
select_character_set_3--=\E+B
select_character_set_3-A=\E+0
select_character_set_3-P=\E+<
##--- 20/20 ---##
select_character_set_3-AI=\E+W
##--- DOWNLOAD CHARACTER SETS ---##
pass=\EP0;1;%p3%d;%p4%d;%p5%d{%s1\E\\
pass=\EP0;1;%p3%d;%p4%d{%s1\E\\
pass=\EP0;1;%p3%d{%s1\E\\
decode_type- =\EP
##--- LINE ATTRIBUTES ---##
line_attribute_off=\E#5
line_attribute=\E#6
line_attribute=\E#3
line_attribute=\E#4
##--- CURSOR (cursor_normal cursor_invisible cursor_visible)
cursor_on=\E[?25h
cursor_off=\E[?25l
### NO cursor_type=
### NO cursor_type_turns_cursor_on
##--- KEYPAD (keypad_xmit keypad_local ) ---##
### keypad_xmit=\E[?1h\E=
### keypad_local=\E[?1l\E>
enter_appl_keypad_mode=\E=
exit_appl_keypad_mode=\E>
enter_cursor_key_mode=\E[?1h
exit_cursor_key_mode=\E[?1l
##--- TABS ---##
### NOTE init_tabs=8
### NO ## ASSUMED tab=
### back_tab=
### NOT_IMP ??clear_tab=\E[g
### NOT_IMP ??clear_tab=\E[0g
clear_all_tabs=\E[3g
set_tab=\EH
##--- FUNCTION KEYS ---##
use=vt220func.fc
use=vt220keys.fc
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
transparent_print_on=\E[5i
transparent_print_off=\E[4i
### TBD print_screen=
##--- STATUS LINE ---##
##--- MISC ---##
### ASSUMED bell=^G
### ONLY IF PADDING carriage_return=^M
### NOT_IMP transparent_underline
new_line=\EE
##--- MULTIPAGE ---##
### NO pages=number
### NO display_page_number=
### NO parm_display_page_next=
### NO parm_display_page_prev=
### NO display_page_next=
### NO display_page_prev=
##--- SPLIT SCREEN ---##
*split_divider= qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq
*split_divider_start=\E(0
*split_divider_end=\E(B
##--- INIT ---##
### NOT_IN_COMMON NO enter_ca_mode=
### NOT_IN_COMMON NO exit_ca_mode=
### SPLITUP reset_2string=\E>\E[?3l\E[?4l\E[?5l\E[?7h\E[?8h
### reset_file=/usr/lib/tabset/vt100
### screen mode h=reverse l=normal
mode-00000000-00000001=\E[?5l
mode-00000001-00000000=\E[?5h
### smooth scroll h=on l=off
mode-00000000-00000002=\E[?4l
mode-00000002-00000000=\E[?4h
### auto repeat h=on l=off
pass=\E[?8h
pass=\E[?8l
### origin mode l=full screen h=in scroll region
ignore=\E[?6l
### newline mode h=lf and cr l=lf
pass=\E[20l
### keyboard lock h=on l=off
ignore=\E[2h
pass=\E[2l
### local echo h=off l=on
pass=\E[12h
### editing mode h=on l=off
pass=\E[?10l
### edit key execution h=immediate l=deferred
pass=\E[?16l
### protected field attributes 0=none 1=bold 2=under 3=blink 7=reverse 254=all
pass=\E[0}
### ansi mode
pass=\E<
use=vt100.fm
