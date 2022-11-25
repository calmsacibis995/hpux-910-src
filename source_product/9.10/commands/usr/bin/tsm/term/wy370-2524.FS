# FacetTerm  wy370-2524   (80 column only)  description file  07/29/91
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: wy370-2524.FS,v 70.1 92/03/09 16:54:07 ssa Exp $
xon_xoff
pass=\E F
##--- MULTIPAGE ---##
# Wyse 370 - 3 pages - 25 lines switchable to 24 - 80 column only
enter_ca_mode=\E[59;3;0;0;0w
enter_ca_mode-25_lines=\E[40h
pages=3
## 
###############################################
columns=80
### columns_wide=132
### columns_wide_on=\E[?3h
### columns_wide_off=\E[?3l
parm_mode_val-NOTIMP..-IGNORE..=?3
### parm_mode_val-COLSWIDE-00000000=?3
### columns_wide_clears_screen
lines=25
## 52_line  52_line=\E[?83h 24_line=\E[?83l
parm_mode_val-NOTIMP..-IGNORE..=?83
###############################################
rows_change-25=\E[40h
rows_change-24=\E[40l
### out_rows_change=
### rows_change_se_on_switch_only
### rows_change_does_clear_screen
### rows_change_turns_auto_wrap_off
### rows_change_turns_auto_wrap_on
### rows_change_switch_page_number_0
### rows_change_clears_pages
### rows_change_sets_cursor_on
### rows_change_resets_insert_mode
### rows_change_resets_appl_keypad_mode
### rows_change_resets_keypad_xmit
### rows_change_resets_scroll_region
rows_change_resets_scroll_region
### rows_change_resets_character_set
### rows_change_attributes_off
### rows_change_resets_save_cursor
### rows_change_resets_origin_mode
### rows_change_sets_mode
###############################################
use=wy370ans.fc
use=wy370com.fc
use=wy370attr.fc
use=wy370pal.fc
use=wy370func.fc
use=wy370keys.fc
use=wy370stat.fc
use=wy370.fm
