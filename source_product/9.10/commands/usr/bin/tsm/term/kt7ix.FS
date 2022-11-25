# FacetTerm  kt7ix ( no xon xoff )  description file 12/10/90
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: kt7ix.FS,v 70.1 92/03/09 16:01:47 ssa Exp $
columns=80
lines=25
graphics_8_bit
### NO (in setup) xon_xoff
### NO auto_left_margin
### ASSUMED auto_right_margin
### NO eat_newline_glitch
### clear unprotected to spaces
clear_screen=$<38>^Z
clear_screen=$<38>\E;
clear_screen=$<38>\E+
### NO clear_all=
### ?? clear_screen_mode_off--P
clr_eol=$<0.4>\Et
clr_eol_w_attr=$<0.4>\ET
clr_eos=$<0.4*>\Ey
clr_eos_w_attr=$<0.4*>\EY
cursor_address=\E=%p1%' '%+%c%p2%' '%+%c
cursor_home=^^
cursor_up=^K
### NO parm_up_cursor=
cursor_down=^V
### NO parm_down_cursor=
cursor_right=^L
### NO parm_right_cursor=
### SAME AS BACKSPACE cursor_left=^H
### NO parm_left_cursor=
### NO change_scroll_region=
### SAME AS NEWLINE scroll_forward=\n
scroll_could_be_cursor_only
scroll_reverse=\Ej
### NO save_cursor=
### NO restore_cursor=
insert_line=$<1.5*>\EE
insdel_line_move_col0
### NO parm_insert_line=
delete_line=$<1.5*>\ER
### NO parm_delete_line=
insert_character=\EQ
### NO insert_padding=
enter_insert_mode=\EZ
exit_insert_mode=\Er
delete_character=\EW
### NO parm_delete_char=
### NO magic_cookie_glitch=0
### HOW DOES THIS WORK - attribute-P=\E)
### HOW DOES THIS WORK - attribute--P=\E(
### NO exit_attribute_mode--PIODBRU=
### NO attribute_next-A-=
### NO flash_screen=
### graphics off
lock_shift_0=\E%
select_character_set_0--=\EG@
select_character_set_0-B-=\EGB
select_character_set_0-R-=\EGD
select_character_set_0-U-=\EGH
select_character_set_0-RB-=\EGF
select_character_set_0-O-=\EG0
select_character_set_0-OB-=\EG2
select_character_set_0-OR-=\EG4
select_character_set_0-OU-=\EG8
### graphics on - \044 is $ - bug in parser
lock_shift_1=\E\044
select_character_set_1-A-=
### NO single_shift_2=string
### NO select_character_set_2--=string
### NO single_shift_3=string
### NO select_character_set_3--=string
auto_wrap_on=\E~
auto_wrap_off=\E0
cursor_on=\E.3
cursor_off=\E.0
### cursor_type=
cursor_type=\E.1
cursor_type=\E.2
cursor_type=\E.4
cursor_type=\E.5
cursor_type=\E.6
### NO exit_appl_keypad_mode=string
### NO exit_cursor_key_mode=string
### NO enter_appl_keypad_mode=string
### NO enter_cursor_key_mode=string
### NO enter_ca_mode=
### NO exit_ca_mode=
###  init_file=/usr/lib/tabset/stdcrt
### NO mode-00000000-00000000=string
### keyboard lock on=\E# off=\E"
ignore=\E#
pass=\E"
### I hear that WP 5.0 use these.
pass=\E(
pass=\E)
### keyclick on=\E> off=\E<
pass=\E>
pass=\E<
### background dark=\Ed bright=\Eb
pass=\Ed
pass=\Eb
### full duplex - half=\E{
pass=\E}
### protect mode on=\E& off=\E'
pass=\E'
### conversation mode on=\EC off=\EB
pass=\EC
### video normal=\EN off=\EO
pass=\EN
### xon_xoff enable=^O disable=^N
pass=^N
### bidirectional printer mode on= off=^T
pass=^T
### monitor mode off=\Eu or \EX on=\EU
pass=\Eu
pass=\EX
### transparent print off=\Ea on=\E` with_display=\E@ without_display=\EA
pass=\Ea
### auto line feed off=\E9 on=\E8
pass=\E9
### margin bell on=\Eo off=\En
pass=\Eo
pass=\En
### ASSUMED bell=^G
### ONLY IF PADDING carriage_return=\r
### NOT_IMP newline=^_
### NOT_IMP print_screen=\EP
pass=\EP
### NOT_IMP prtr_non=
### NOT_IMP prtr_off=\Ea
### NOT_IMP prtr_on=\E`
### NOT_IMP back_tab=\EI
clear_all_tabs=\E3
### ALSO ?clear_tab?=\E2
pass=\E2
set_tab=\E1
key_home=^^
key_up=\E[A
key_down=\E[B
key_left=\E[D
key_right=\E[C
### SAME_AS_BACKSPACE key_backspace=^H
### NOT USED key_f0=^AI\r
### NOT USED key_f1=^A@\r
### NOT USED key_f2=^AA\r
### NOT USED key_f3=^AB\r
### NOT USED key_f4=^AC\r
### NOT USED key_f5=^AD\r
### NOT USED key_f6=^AE\r
### NOT USED key_f7=^AF\r
### NOT USED key_f8=^AG\r
### NOT USED key_f9=^AH\r
use=kt7ixkeys.fc
use=kt7ix.fm
