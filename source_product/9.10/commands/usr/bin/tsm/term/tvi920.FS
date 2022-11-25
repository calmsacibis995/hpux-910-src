# FacetTerm  tvi920  description file 06/22/88
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: tvi920.FS,v 70.1 92/03/09 16:04:49 ssa Exp $
columns=80
lines=24
### ?? graphics_8_bit
xon_xoff
##--- MARGINS ---##
### NO auto_left_margin
### ASSUMED auto_right_margin
### NO eat_newline_glitch
### NO auto_wrap_on=
### NO auto_wrap_off=
##--- CLEAR ---##
clear_screen--P=\E*
clear_screen--P=\E+
clear_screen--=^Z
### ?? clear_all=^Z
### ?? clear_screen_mode_off--P
clr_eol=\Et
clr_eol_w_attr=\ET
clr_eos=\Ey
clr_eos_w_attr=\EY
##--- CURSOR ---##
cursor_address=\E=%p1%' '%+%c%p2%' '%+%c
cursor_home=^^
cursor_up=^K
### NO parm_up_cursor=
cursor_down=^V
### NO parm_down_cursor=
cursor_right=^L
### NO parm_right_cursor=
### SAME_AS_BACKSPACE cursor_left=^H
### NO parm_left_cursor=
##--- SCROLL ---##
### NO change_scroll_region=
### SAME_AS_NEWLINE scroll_forward=\n
### ?? scroll_could_be_cursor_only
### NO scroll_reverse=
### NO save_cursor=
### NO restore_cursor=
##--- INS/DEL ---##
insert_line=\EE$<5*>
insdel_line_move_col0
### NO parm_insert_line=
delete_line=\ER$<5*>
### NO parm_delete_line=
insert_character=\EQ
### NO insert_padding=
### NO enter_insert_mode=
### NO exit_insert_mode=
delete_character=\EW
### NO parm_delete_character=
##--- ATTRIBUTES ---##
### NOTE magic_cookie_glitch=1
### NO ## USE_MAGIC attribute-A=
### NO ## USE_MAGIC attribute--A=
### ?? attribute_next-A-=
### NO ## USE_MAGIC attribute-B=
### NO ## USE_MAGIC attribute-O=
### NO ## USE_MAGIC attribute-D=
attribute-P=\E)
attribute--P=\E(
### NO ## USE_MAGIC attribute-R=
### NO ## USE_MAGIC attribute-I=
### USE_MAGIC attribute-?=\Ej
### USE_MAGIC attribute--?=\Ek
### USE_MAGIC attribute-U=\El
### USE_MAGIC attribute--U=\Em
### NO ## USE_MAGIC exit_attribute_mode--APIODBRU=
##--- MAGIC COOKIE ATTRIBUTES ---##
### blink ^ q  secure _ q  standout j k   underline l m
magic=\E\136
magic=\E_
nomagic=\Eq
magic=\Ej
nomagic=\Ek
magic=\El
nomagic=\Em
### alt_charset
### NO magic=
### blink
magic=\EG2
### bold
### NO magic=
### dim
magic=\EGp
### protected
### NO magic=
### reverse
magic=\EG4
### secure
magic=\EG1
### standout
### REVERSE magic=\EG4
### underline
magic=\EG8
### reverse-blink 6 underline-blink : dim-blink r dim-reverse t 
magic=\EG6
magic=\EG:
magic=\EGr
magic=\EGt
### dim-reverse-blink v dim-underline x dim-underline-blink z
magic=\EGv
magic=\EGx
magic=\EGz
### exit_attribute_mode
nomagic=\EG0
### exit_alt_charset_mode
### NO nomagic=
### exit_standout_mode
### exit_underline_mode
### NO flash_screen=
##--- CHARACTER SET ATTRIBUTES ---##
### ?? lock_shift_0=
### ?? select_character_set_0--=
### ?? select_character_set_0--=
### ?? lock_shift_1=
### ?? select_character_set_1--=
### ?? select_character_set_1--=
### ?? single_shift_2=
### ?? select_character_set_2--=
### ?? single_shift_3=
### ?? select_character_set_3--=
##--- CURSOR (cursor_normal=\E.2 cursor_invisible cursor_visible=\E.4)
cursor_on=\E.1
cursor_off=\E.0
cursor_type=\E.2
cursor_type=\E.3
cursor_type=\E.4
cursor_type=\E.5
cursor_type_turns_cursor_on
##--- KEYPAD (keypad_xmit keypad_local ) ---##
### NO enter_appl_keypad_mode=
### NO exit_appl_keypad_mode=
### NO enter_cursor_key_mode=
### NO exit_cursor_key_mode=
##--- TABS ---##
### ASSUMED tab=^I
### back_tab=
clear_all_tabs=\E3
set_tab=\E1
##--- FUNCTION KEYS ---##
### key_home=
key_up=^K
### key_down=
key_right=^L
### key_left=
### key_backspace=
### TBD key_f0=^A@\r
### TBD key_f1=^AA\r
### TBD key_f2=^AB\r
### TBD key_f3=^AC\r
### TBD key_f4=^AD\r
### TBD key_f5=^AE\r
### TBD key_f6=^AF\r
### TBD key_f7=^AG\r
### TBD key_f8=^AH\r
### TBD key_f9=^AI\r
### NO ## TBD key_f10=
##--- PRINTER ---##
### NOT_ON_920 transparent_print_off=\Ea
### NOT_ON_920 transparent_print_on=\E`
### exten'n print on=\E@  off=\EA
pass=\EA
### NOT_ON_920 bidirectional print  on=^R  off=^T
### NOT_ON_920 pass=^T
##--- STATUS LINE ---##
##--- MISC ---##
### ASSUMED bell=^G
### ONLY IF PADDING carriage_return=\r
### NO ## OK_IF_MAGIC transparent_underline
##--- INIT ---##
### Auto_page off=\Ew  on=\Ev
pass=\Ew
### cursor_postion page0-24=\E-07\s  page1-24=\E-17\s 
ignore=\E-07\040
ignore=\E-17\040
enter_ca_mode=\Ew
### NO exit_ca_mode=
### ?? mode-00000000-00000000=
### Flash screen
mode-00000000-00000002=\Ed$<20/>
mode-00000002-00000000=\Eb$<20/>
### ?? ignore=
### ?? pass=
### Keyboard lock=\E# unlock=\E"
ignore=\E#
pass=\E"
### protect mode off=\E'
pass=\E'
use=tvi.fm
