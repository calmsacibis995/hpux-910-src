# FacetTerm  Wy370-2451   (80/132 column)  description file  09/10/90
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: Wy370-2451.FS,v 70.1 92/03/09 16:48:32 ssa Exp $
xon_xoff
pass=\E F
##--- MULTIPAGE ---##
# Wyse 370 with 1 page switchable 80/132 columns 24/25/50/51 lines
### mode=wy370                      pages cols 80/132/161 lines 24/50
enter_ca_mode-............=\E[90;1"p\E[59;1;1;1;0w
enter_ca_mode-80_columns..=\E[?3l
enter_ca_mode-24_lines....=\E[?83l\E[40l\E[1;24r
pages=1
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
###############################################
rows_change-24=\E[?83l
out_rows_change_extra_data_row_off=\E[?83l\E[1;24r
out_rows_change_extra_data_row_on=\E[?83l\E[1;25r
rows_change-50=\E[?83h
out_rows_change_extra_data_row_off=\E[?83h\E[1;50r
out_rows_change_extra_data_row_on=\E[?83h\E[1;51r
extra_data_row_off=\E[40l
out_extra_data_row_off-0=\E[40l\E[1;24r
out_extra_data_row_off-1=\E[40l\E[1;50r
extra_data_row_on=\E[40h
out_extra_data_row_on-0=\E[40h\E[1;25r
out_extra_data_row_on-1=\E[40h\E[1;51r
out_clr_memory_below=\E[J
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
extra_data_row_resets_scroll_region
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
use=wy370graph.fc
