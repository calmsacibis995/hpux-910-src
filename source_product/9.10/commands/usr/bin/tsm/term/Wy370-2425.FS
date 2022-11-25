# FacetTerm  Wy370-2425   (80/132 column)  description file  07/29/91
## Copyright (c) Structured Software Solutions, Inc. 1991.  All Rights Reserved.
## @(#) $Header: Wy370-2425.FS,v 70.1 92/03/09 16:48:29 ssa Exp $
xon_xoff
pass=\E F
##--- MULTIPAGE ---##
### pages cols 80/132/161 lines 24/50
# Wyse 370 - 2 pages - 24 lines switchable to 25 - 80 column switchable to 132
enter_ca_mode=\E[59;2;1;0;0w
enter_ca_mode-24_line=\E[40l
pages=2
## 
###############################################
columns=80
columns_wide=132
columns_wide_on=\E[?3h
columns_wide_off=\E[?3l
### parm_mode_val-NOTIMP..-IGNORE..=?3
parm_mode_val-COLSWIDE-00000000=?3
### columns_wide_clears_screen
enter_ca_mode-width_change_clear_off=\E[35h
lines=24
## 52_line  52_line=\E[?83h 24_line=\E[?83l
parm_mode_val-NOTIMP..-IGNORE..=?83
###############################################
rows_change-24=\E[40l
rows_change-25=\E[40h
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
