# FACET/TERM  7900+  description file  8/16/87
## @(#) $Header: 7900+.FS,v 66.2 90/09/20 12:28:47 kb Exp $
columns=80
lines=24
### NO graphics_8_bit
### NO xon_xoff
### DOES TERMINAL HAVE auto_left_margin
### ASSUMED auto_right_margin
### NO eat_newline_glitch
clear_screen=^L
### NO clear_all=
clr_eol=\EK
### NO clr_eos_w_attr=
clr_eos=\Ek
### NO clr_eos_w_attr=
cursor_address=\E1%p2%c%p1%'0'%+%c
cursor_home=^A
cursor_up=^Z
### NO parm_up_cursor=
### SAME AS NEWLINE - cursor_down=\n
### NO parm_down_cursor=
cursor_right=^F
### NO parm_right_cursor=
cursor_left=^U
### NO parm_left_cursor=
change_scroll_region=%i\Ep%p1%d;%p2%d\r
### SAME AS NEWLINE - scroll_forward=\n
### ?? scroll_could_be_cursor_only
### NO scroll_reverse=
### NO save_cursor=
### NO restore_cursor=
insert_line=\EM
### ?? insdel_line_move_col0
### NO parm_insert_line=
delete_line=\El
### NO parm_delete_line=
insert_character=\EF
### NO insert_padding=
### NO enter_insert_mode=
### NO exit_insert_mode=
delete_character=\EE
### NO parm_delete_char=
### AFFECTS magic_cookie_glitch=1 but not alt_charset
attribute-A=\E(
attribute--A=\E)
### blink dim reverse underline
magic=\E0B
magic=\E0A
magic=\E0P
magic=\E0`
magic=\E0R
magic=\E0C
magic=\E0Q
magic=\E0S
nomagic=\E0@
### NO attribute_next-A-=
### NO flash_screen=
### NO lock_shift_0=
### NO select_character_set_0--=
### NO lock_shift_1=
### NO select_character_set_1--=
### NO single_shift_2=
### NO select_character_set_2--=
### NO single_shift_3=
### NO select_character_set_3--=
### ?? auto_wrap_on=
### ?? auto_wrap_off=
cursor_on=^X
cursor_off=^W
### NO cursor_type=
### NO cursor_type_turns_cursor_on
### NO exit_appl_keypad_mode=
### NO exit_cursor_key_mode=
### NO enter_appl_keypad_mode=
### NO enter_cursor_key_mode=
enter_ca_mode=^L
### NO exit_ca_mode=
### DOES_TERMINAL_NEED mode-00000000-00000000=
### DOES_TERMINAL_NEED ignore=
### DOES_TERMINAL_NEED pass=
### ASSUMED bell=^G
### ONLY IF PADDING carriage_return=\r
### NO NOT_IMP init_tabs=number
### NO NOT_IMP back_tab=
### NO NOT_IMP clear_all_tabs=
### NO NOT_IMP set_tab=
### NOT_USED key_clear=^L
### NOT_USED key_f0=^B0\n
### NOT_USED key_f1=^B1\n
### NOT_USED key_f2=^B2\n
### NOT_USED key_f3=^B3\n
### NOT_USED key_f4=^B4\n
### NOT_USED key_f5=^B5\n
### NOT_USED key_f6=^B6\n
### NOT_USED key_f7=^B7\n
### NOT_USED key_f8=^B8\n
### NOT_USED key_f9=^B9\n
### NOT_USED key_home=^A
### key_up=^Z
### SAME_AS_NEWLINE key_down=\n
### key_left=^U
### key_right=^F
### SAME_AS_BACKSPACE key_backspace=^H
