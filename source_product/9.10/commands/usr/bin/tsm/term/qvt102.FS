# FacetTerm  qvt102  description file 10/04/88
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: qvt102.FS,v 70.1 92/03/09 16:02:37 ssa Exp $
columns=80
lines=24
### NO graphics_8_bit
xon_xoff
auto_left_margin
### ASSUMED auto_right_margin
### NO eat_newline_glitch
### clear all unprotected to spaces
clear_screen=^Z
###?	### clear all to nulls - protect off
###?	clear_screen=\E*
###?	clear_screen_mode_off--P
###?	clr_eol=\Et
clr_eol=\ET
###?	clr_eol_w_attr=\ET
###?	clr_eos=\Ey
clr_eos=\EY
###?	clr_eos_w_attr=\EY
cursor_address=\E=%p1%' '%+%c%p2%' '%+%c
cursor_home=^^
cursor_up=^K
###?	## NO parm_up_cursor=
### SAME AS NEWLINE cursor_down=\n
###?	## NO parm_down_cursor=
cursor_right=^L
###?	## NO parm_right_cursor=
###?	## SAME AS BACKSPACE cursor_left=^H
###?	## NO parm_left_cursor=
###?	## NO change_scroll_region=
###?	## SAME AS NEWLINE scroll_forward=\n
###?	## NO scroll_could_be_cursor_only
###?	## NO scroll_reverse=
###?	## NO save_cursor=
###?	## NO restore_cursor=
insert_line=\EE
insdel_line_move_col0
###?	## NO parm_insert_line=
delete_line=\ER
###?	## NO parm_delete_line=
insert_character=\EQ
###?	## NO insert_padding=
### NO enter_insert_mode=
### NO exit_insert_mode=
delete_character=\EW
###?	## NO parm_delete_char=
### NOTE magic_cookie_glitch=1
### reverse underline
magic=\EG4
magic=\EG8
nomagic=\EG0
### GRAPHICS
attribute-A=\E$
attribute--A=\E%
### NO attribute_next-A-=
### REVERSE VIDEO WITH PROTECT \E` = reverse protect char
attribute-P=\E)
attribute--P=\E(
pass=\E'
### NO exit_attribute_mode--APIODBRU=
###?	flash_screen=\EA04\0\0\0\0\0\0\0\0\0\0\0\EA00
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
### NO auto_wrap_on=
### NO auto_wrap_off=
###?	cursor_off=\E`0
###?	cursor_on=\E`1
###?	cursor_type=\E`3
###?	cursor_type=\E`2
###?	cursor_type=\E`4
###?	cursor_type=\E`5
###?	cursor_type_turns_cursor_on
###?	## NO exit_appl_keypad_mode=
###?	## NO exit_cursor_key_mode=
###?	## NO enter_appl_keypad_mode=
###?	## NO enter_cursor_key_mode=
###?	## NO enter_ca_mode=
###?	## NO exit_ca_mode=
###?	## NO mode-00000000-00000000=
###?	## NO ignore=
###?	## local message field blank
###?	pass=\EA21
###?	### host message field
###?	pass=\EF%s1\r
###?	### reverse protect character   A=normal 7=dim
###?	pass=\E`6
###?	### keyboard unlock=\E" lock=\E#
###?	ignore=\E#
pass=\E"
### mode conversational=\EC     ?block=\EB
pass=\EC
###?	### duplex full=\EDF half=\EDH
###?	pass=\EDF
### monitor ?off=\Eu off=\EX ?on=\EU
###?	pass=\Eu
pass=\EX
###?	### no scroll  off=\EO  on=\EN
###?	pass=\EO
###?	### protect submode  off&autoscrollon=\E'  on&autoscrolloff=\E&
###?	pass=\E'
###?	### columns 80=\E`:  132=\E`;
###?	pass=\E`:
###?	## ASSUMED bell=^G
###?	## ONLY IF PADDING carriage_return=\r
###?	## NOT_IMP init_tabs=number
### NOT_IMP back_tab=\EI
###?	## NOT_IMP clear_all_tabs=\E0
###?	pass=\E0
###?	## NOT_IMP set_tab=\E1
###?	pass=\E1
key_home=^^
key_up=^K
### SAME_AS_NEWLINE key_down=\n
### SAME_AS_BACKSPACE key_left=^H
key_right=^L
### SAME_AS_BACKSPACE key_backspace=^H
###? NOT USED key_clear=\EY
### NOT USED key_f0=^A@\r
### NOT USED key_f1=^AA\r
### NOT USED key_f2=^AB\r
### NOT USED key_f3=^AC\r
### NOT USED key_f4=^AD\r
### NOT USED key_f5=^AE\r
### NOT USED key_f6=^AF\r
### NOT USED key_f7=^AG\r
pass=\E{
### RWORD INIT
pass=\EA
pass=\E^T
use=qvt.fm
