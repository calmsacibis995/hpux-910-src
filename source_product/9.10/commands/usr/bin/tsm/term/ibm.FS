# FacetTerm  ibm hft  description file 10/15/90
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: ibm.FS,v 70.1 92/03/09 15:58:14 ssa Exp $
##### hft|Generic HFT display device - no colors supported
##### 	. cr=^M
##### 	. cud1=^J
##### 	ind=\E[S
##### 	. bel=^G
##### 	il1=\E[L
##### 	. am
##### 	. cub1=^H
##### 	ed=\E[J
##### 	el=\E[K
##### 	clear=\E[H\E[J
##### 	cup=\E[%i%p1%d;%p2%dH
##### 	cols#80
##### 	lines#25
##### 	dch1=\E[P
##### 	dl1=\E[M
##### 	home=\E[H
##### 	dl=\E[%p1%dM
##### 	il=\E[%p1%dL
##### 	smir=\E[4h
##### 	rmir=\E[4l
##### 	bold=\E[1m
##### 	rev=\E[7m
##### 	blink=\E[5m
##### 	invis=\E[8m
##### 	sgr0=\E[0m
##### 	? sgr=\E[%?%p1%t7;%;%?%p2%t4;%;%?%p3%t7;%;%?%p4%t5;%;%?%p6%t1;%;m
##### 	kcuu1=\E[A
##### 	kcud1=\E[B
##### 	kcub1=\E[D
##### 	kcuf1=\E[C
##### 	khome=\E[H
##### 	. kbs=^H
##### 	cuf1=\E[C
##### 	ht=^I
##### 	cuu1=\E[A
##### 	xon
##### 	rmul=\E[m
##### 	smul=\E[4m
##### 	rmso=\E[m
##### 	smso=\E[7m
##### 	X kpp=\E[150q
##### 	X knp=\E[154q
##### 	X kf1=\E[001q
##### 	X kf2=\E[002q
##### 	X kf3=\E[003q
##### 	X kf4=\E[004q
##### 	X kf5=\E[005q
##### 	X kf6=\E[006q
##### 	X kf7=\E[007q
##### 	X kf8=\E[008q
##### 	X kf9=\E[009q
##### 	X kf10=\E[010q
##### 	X kf11=\E[011q
##### 	X kf12=\E[012q
##### 	X kf13=\E[013q
##### 	X kf14=\E[014q
##### 	X kf15=\E[015q
##### 	X kf16=\E[016q
##### 	X kf17=\E[017q
##### 	X kf18=\E[018q
##### 	X kf19=\E[019q
##### 	X kf20=\E[020q
##### 	X kf21=\E[021q
##### 	X kf22=\E[022q
##### 	X kf23=\E[023q
##### 	X kf24=\E[024q
##### 	X kf25=\E[025q
##### 	X kf26=\E[026q
##### 	X kf27=\E[027q
##### 	X kf28=\E[028q
##### 	X kf29=\E[029q
##### 	X kf30=\E[030q
##### 	X kf31=\E[031q
##### 	X kf32=\E[032q
##### 	X kf33=\E[033q
##### 	X kf34=\E[034q
##### 	X kf35=\E[035q
##### 	X kf36=\E[036q
##### 	X kf37=\E[037q
##### 	X kf38=\E[038q
##### 	X kf39=\E[039q
##### 	X kf40=\E[040q
##### 	X kf41=\E[041q
##### 	X kf42=\E[042q
##### 	X kf43=\E[043q
##### 	X kf44=\E[044q
##### 	X kf45=\E[045q
##### 	X kf46=\E[046q
##### 	X kf47=\E[047q
##### 	X kf48=\E[048q
##### 	bw
##### 	. it#8
##### 	. msgr
##### 	hpa=\E[%i%p1%dG
##### 	ech=\E[%p1%dX
##### 	X kend=\E[146q
##### 	X ktab=^I
##### 	X kdch1=\E[P
##### 	X kind=\E[151q
##### 	X kich1=\E[139q
##### 	X krmir=\E[4l
##### 	X knl=^M
##### 	X kbtab=\E[Z
##### 	X kri=\E[155q
##### 	cub=\E[%p1%dD
##### 	cuf=\E[%p1%dC
##### 	indn=\E[%p1%dS
##### 	rin=\E[%p1%dT
##### 	ri=\e[T
##### 	cuu=\E[%p1%dA
##### 	cud=\E[%p1%dB
##### 	X box1=\332\304\277\263\331\300\302\264\301\303\305
##### 	X box2=\311\315\273\272\274\310\313\271\312\314\316
##### 	X batt2=md
##### 	X dispc=%?%p2%{32}%<%t%{31}%c%p2%{160}%+%c%e%p2%{224}%<%t%p2%c%e%p2%{230}%<%t%{28}%c%p2%{42}%+%c%e%p2%{231}%<%t%p2%c%e%p2%{241}%<%t%{28}%c%p2%{87}%-%c%e%p2%{241}%=%t%p2%c%e%p2%{246}%<%t%{28}%c%p2%{88}%-%c%e%p2%{247}%=%t%{28}%c%{158}%c%e%p2%{249}%=%t%{28}%c%{159}%c%e%p2%{251}%=%t%{28}%c%{160}%c%e%p2%{252}%=%t%{28}%c%{162}%c%e%p2%c%;
##### #
##### #########################################################################
##### #       HFT Display Device to support color
##### #########################################################################
##### #
##### hft-c|HFT with Color,
##### 	colf0=\E[30m,   colf1=\E[31m,   colf2=\E[32m,   colf3=\E[33m,
##### 	colf4=\E[34m,   colf5=\E[35m,   colf6=\E[36m,   colf7=\E[37m,
##### 	colb0=\E[40m,   colb1=\E[41m,   colb2=\E[42m,   colb3=\E[43m,
##### 	colb4=\E[44m,   colb5=\E[45m,   colb6=\E[46m,   colb7=\E[47m,
##### 	use=hft,
##### #
## xon_xoff
xon_xoff
## graphics_escape_control
## graphics_escape_delete
## columns=80
columns=80
## columns_wide=132
## columns_wide_mode_on_default
## columns_wide_on=
## columns_wide_off=
## columns_wide_clears_screen
## columns_wide_mode_on_mode_bit-08000000
## parm_mode_val-COLSWIDE-00000000=
## lines=24
lines=25
## ##--- MARGINS ---##
## auto_left_margin
auto_left_margin
## eat_newline_glitch
## real_eat_newline_glitch
## auto_wrap_on=
## auto_wrap_off=
## auto_wrap_mode_bit-04000000
## parm_mode_val-AUTOWRAP-00000000=
## set_margins=
## ##--- CLEAR ---##
## ### NOTE: write protect mode is supported for clear_screen_unprotected only
## write_protect_on=
## write_protect_off=
## clear_screen_unprotected--=
## clear_screen_unprotected_w_attr--=
## clear_screen--P=
## clr_eos=
clr_eos=\E[J
## out_clr_eos=
## clr_eos_w_attr=
## clr_eos_chars=
## set_attr_eos=
## clr_eos_chars_erasable-=
## clr_bos=
## clr_bos_chars_erasable-=
## clear_all=
## clear_all_chars_erasable-=
## clr_eol=
clr_eol=\E[K
## out_clr_eol=
## clr_eol_w_attr=
## clr_eol_chars=
## set_attr_eol=
## out_clr_eol_chars_erasable=
## clr_bol=
## out_clr_bol=
## clr_bol_w_attr=
## clr_bol_chars=
## set_attr_bol=
## out_clr_bol_chars_erasable=
## clear_line=
## out_clear_line=
## clear_line_w_attr=
## clear_line_chars=
## set_attr_line=
## erase_chars=
erase_chars=\E[%p1%dX
## ##--- CURSOR ---##
## cursor_address=
cursor_address=\E[%i%p1%d;%p2%dH
## cursor_address_wide=
## cursor_address_wide_starts=number
## row_address=
## column_address=
column_address=\E[%i%p1%dG
## decode_type-0123456789;?=\E[
## cursor_home=
cursor_home=\E[H
## cursor_up=
cursor_up=\E[A
## cursor_up_at_home_wraps_ll
## parm_up_cursor=
parm_up_cursor=\E[%p1%dA
## cursor_down=
cursor_down=\E[B
## parm_down_cursor=
parm_down_cursor=\E[%p1%dB
## cursor_right=
cursor_right=\E[C
## parm_right_cursor=
parm_right_cursor=\E[%p1%dC
## cursor_left=
cursor_left=\E[D
## parm_left_cursor=
parm_left_cursor=\E[%p1%dD
## cursor_to_ll=
## ##--- SCROLL ---##
## change_scroll_region=
## scroll_forward=
scroll_forward=\E[S
## scroll_could_be_cursor_only
## parm_index=
parm_index=\E[%p1%dS
## scroll_reverse=
scroll_reverse=\E[T
## scroll_reverse_move_col0
## parm_rindex=
parm_rindex=\E[%p1%dT
## save_cursor=
## restore_cursor=
## auto_scroll_on=
## auto_scroll_off=
## auto_scroll_off_wraps_top
## memory_lock=
## memory_unlock=
## ##--- INS/DEL ---##
## insert_line=
insert_line=\E[L
## out_insert_line=
## insdel_line_move_col0
## insert_line_needs_clear_glitch
## insert_line_sets_attributes
## parm_insert_line=
parm_insert_line=\E[%p1%dL
## out_parm_insert_line=
## delete_line=
delete_line=\E[M
## parm_delete_line=
parm_delete_line=\E[%p1%dM
## insert_character=
## insert_padding=
## parm_ich=
## enter_insert_mode=
enter_insert_mode=\E[4h
## exit_insert_mode=
exit_insert_mode=\E[4l
## insert_mode_on_mode_bit-02000000
## parm_mode_val-INSERTON-00000000=
## delete_character=
delete_character=\E[P
## parm_delete_character=
## ##--- ATTRIBUTES URBDOIPA ---##
## attribute-U=
## attribute--U=
## attribute-R=
## attribute-B=
## attribute-D-O=
## attribute-O-D=
## attribute-I=
## attribute-P=
## attribute-A=
## attribute--A=
## attribute_next-A-=
## ##--- MULTIPLE ATTRIBUTES URBDOIPA ---##
## ##---                              ---##
## parm_attribute=\E[%7MMm
parm_attribute=\E[%7MMm
parm_attribute=\E[%7MM;m
## parm_attribute--URBDOIPA=\E[;%7MMm
parm_attribute--URBDOIPA-=\E[;%7MMm
parm_attribute--URBDOIPA-=\E[;%7MM;m
## parm_attribute_out=\E[%s1m
parm_attribute_out=\E[%s1m
parm_attribute_out=\E[%s1;%s2m
parm_attribute_out=\E[%s1;%s2;%s3m
parm_attribute_out=\E[%s1;%s2;%s3;%s4m
parm_attribute_out=\E[%s1;%s2;%s3;%s4;%s5m
parm_attribute_out=\E[%s1;%s2;%s3;%s4;%s5;%s6m
parm_attribute_out=\E[%s1;%s2;%s3;%s4;%s5;%s6;%s7m
parm_attribute_out=\E[%s1;%s2;%s3;%s4;%s5;%s6;%s7;%s8m
## parm_attribute_out=\E[%s1;%s2;%s3;%s4;%s5;%s6;%s7;%s8m
##### 	bold=\E[1m
##### 	rev=\E[7m
##### 	blink=\E[5m
##### 	invis=\E[8m
##### 	sgr0=\E[0m
## parm_attribute_val--URBDOIPA=0
parm_attribute_val--URBDOIPA=0
## parm_attribute_val-U-=number
parm_attribute_val-U-=4
## parm_attribute_val-R-=number
parm_attribute_val-R-=7
## parm_attribute_val-B-=number
parm_attribute_val-B-=5
## parm_attribute_val-D-O=number
parm_attribute_val-D-O=number
## parm_attribute_val-O-D=number
parm_attribute_val-O-D=1
## parm_attribute_val-I-=8
parm_attribute_val-I-=number
## parm_attribute_val-P-=number
parm_attribute_val-P-=number
## parm_attribute_val-A-=number
parm_attribute_val-A-=number
parm_attribute_val-....-lkji=32
parm_attribute_val-...i-lkj.=30
parm_attribute_val-..j.-lk.i=31
parm_attribute_val-.k..-l.ji=33
parm_attribute_val-.k.i-l.j.=34
parm_attribute_val-.kj.-l..i=35
parm_attribute_val-.kji-l...=36
parm_attribute_val-l...-.jki=37
parm_attribute_val-....-ponm=40
parm_attribute_val-..n.-po.m=41
parm_attribute_val-..nm-po..=42
parm_attribute_val-.o..-p.nm=43
parm_attribute_val-.o.m-p.n.=44
parm_attribute_val-.on.-p..m=45
parm_attribute_val-.onm-p...=46
parm_attribute_val-p...-.onm=47
## exit_attribute_mode--URBDOIPA=
exit_attribute_mode--URBDOIPA=\E[0m
exit_attribute_mode--URBDOIPA=\E[m
## ##--- MAGIC COOKIE ATTRIBUTES ---##
## magic-standout=
## magic-underline=
## magic-reverse=
## magic-blink=
## magic-dim=
## magic-bold=
## magic-secure=
## magic-protected=
## magic-alt_charset=
## nomagic-exit_attribute_mode=
## ##--- CHARACTER SET ATTRIBUTES URBDOIPA ---##
## ##---                                   ---##
## select_character_set_noload
## lock_shift_0=
## single_shift_0=
## select_character_set_0--=
## select_character_set_0--=
## lock_shift_1=
## single_shift_1=
## select_character_set_1--=
## select_character_set_1--=
## lock_shift_2=
## single_shift_2=
## select_character_set_2--=
## select_character_set_2--=
## lock_shift_3=
## single_shift_3=
## select_character_set_3--=
## select_character_set_3--=
## ##--- LINE ATTRIBUTES ---##
## line_attribute_off=
## line_attribute=
## line_attribute=
## ##--- CURSOR (cursor_normal cursor_invisible cursor_visible)
## cursor_on=
## cursor_off=
## cursor_on_mode_bit-00800000
## parm_mode_val-CURSORON-00000000=
## cursor_type=
## cursor_type=
## cursor_type_turns_cursor_on
## ##--- KEYPAD (keypad_xmit keypad_local ) ---##
## enter_appl_keypad_mode=
## exit_appl_keypad_mode=
## appl_keypad_mode_on_mode_bit-00400000
## parm_mode_val-APPKEYON-00000000=
## enter_cursor_key_mode=
## exit_cursor_key_mode=
## cursor_key_mode_on_mode_bit-00200000
## parm_mode_val-CURKEYON-00000000=
## keypad_xmit=
## keypad_local=
## keypad_xmit_mode_bit-00100000
## parm_mode_val-KEYPADXM-00000000=
## ##--- TABS ---##
## clear_tab=
## clear_all_tabs=
## set_tab=
## back_tab=
## ##--- FUNCTION KEYS ---##
## function_key_clear=
## function_key_clear_mode_yes=number
## function_key_clear_mode_no=number
## parm_pkey_key=
## parm_pkey_key=
## pkey_key=
## out_pkey_key=
## function_key_string_is_hex
## function_key_string_is_ansi
## function_key_string_is_escaped-chars=
## function_key_includes_label
## function_key_len_0_label_only
## function_key-id-delim=
## function_key-id-delim=
## shift_function_key
## function_key-id-delim=
## function_key-id-delim=
## ##--- CURSOR KEYS ---##
## key_home=
key_home=\E[H
## key_up=
key_up=\E[A
## key_down=
key_down=\E[B
## key_right=
key_right=\E[C
## key_left=
key_left=\E[D
## key_backspace=
## ##--- PRINTER ---##
## transparent_print_on=
## transparent_print_off=
## # TBD print_screen=
## out_transparent_print=
## hp_transparent_print_reply
## ##--- STATUS LINE ---##
## status_off=
## status_on=
## status_off_clears_status_type
## status_off_clears_status_line
## status_type=
## status_type=
## status_type_turns_status_on
## status_type_ruins_status_line
## status_line=
## status_line_turns_status_on
## status_line_clears_status_line_labels
## status_off_clears_status_line_labels
## status_line_label=\Ez0%8s1\r
## status_line_label=\Ez1%8s1\r
## not_imp-other_labels=\Ez%p1%c%8s1\r
## onstatus=
## out_onstatus=
## onstatus_clear=
## ##--- MISC ---##
## # ONLY IF PADDING carriage_return=\r$<20/>
## # ONLY IF NOT \n  new_line=
## ##--- CONTROL_8_BIT ---##
## control_8_bit
## graphics_8_bit
## input_8_bit
## force_CS8
## no_clear_ISTRIP
## terminal_mode_default=number
## terminal_mode_name-CG=
## terminal_mode_select-0-RP=
## out_terminal_mode_switch-0-RP=
## out_terminal_mode_select=
## ##--- PC_MODE ---##
## pc_mode_on=
## pc_mode_off=
## pc_mode_kd_scancode_driver
## out_pc_mode_on_1=
## out_pc_mode_on_2=
## out_pc_mode_on_3=
## out_pc_mode_off_1=
## out_pc_mode_off_2=
## out_pc_mode_off_3=
## pc_mode_se_on_switch_only
## pc_mode_does_clear_screen
## pc_mode_on_turns_auto_wrap_off
## pc_mode_off_turns_auto_wrap_on
## pc_mode_on_switch_page_number_0
## pc_mode_off_switch_page_number_0
## pc_auto_wrap_on=
## pc_auto_wrap_off=
## pc_cursor_on=
## pc_cursor_off=
## pc_cursor_type=
## pc_cursor_type=
## ##--- PASTE ---##
## paste_eol-type=
## ##--- GRAPH_MODE ---##
## graph_mode_start=
## graph_mode_clear=
## out_graph_mode_clear=
## ##--- MULTIPAGE ---##
## pages=number
## display_page_number=
## parm_display_page_next=
## parm_display_page_prev=
## display_page_next=
## display_page_prev=
## ##--- SPLIT SCREEN ---##
## ### NOTE: split_divider must be 77 char starting with blank
## split_divider= qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq
## ### NOTE: split_divider_end must reverse split_divider_start to default
## split_divider_start=
## split_divider_end=
## pc_split_divider= \315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315\315
## pc_split_divider_start=
## pc_split_divider_end=
## ##--- INIT ---##
## enter_ca_mode=
## enter_ca_mode_pc=
## enter_ca_mode_pc_2=
## enter_ca_mode_2=
## exit_ca_mode=
## exit_ca_mode_pc=
## exit_ca_mode_pc_2=
## exit_ca_mode_2=
## ##--- PARM_MODE ---##
## parm_mode_on=\E[%10M?h
## parm_mode_off=\E[%10M?l
## parm_mode_private_propogates
## mode-bits_on.-bits_off=
## mode-00000000-00000001=\E[?18l
## mode-00000001-00000000=\E[?18h
## parm_mode_val-h.bitson-l.bitson=
## parm_mode_val-00000001-00000000=?18
## multi_mode-bitmask.-bitvalue=
## multi_mode-0000000C-00000000=\E`6$<20>
## multi_mode-0000000C-00000004=\E`A$<20>
## multi_mode-0000000C-00000008=\E`7$<20>
## ignore_mode_bit-80000000
## parm_mode_val-80000000-IGNORE..=?10
## pass_mode_bit-40000000
## parm_mode_val-40000000-PASS....=?11
## notimp_mode_bit-20000000
## parm_mode_val-20000000-NOTIMP..=?12
## ignore=
## ignore_parm=
## pass=
## ##--- MODE ---##
use=ibm.fm
