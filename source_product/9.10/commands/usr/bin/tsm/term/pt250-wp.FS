# FacetTerm  pt250-wp  description file 06/12/91
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: pt250-wp.FS,v 70.1 92/03/09 16:50:27 ssa Exp $
lines=24
graphics_8_bit
graphics_escape_control
enter_ca_mode=\E[>25l
enter_ca_mode_2=\E[1Q\E[1;24y\E[12;>7;>10;>13h\E[4;>1;>3;>8;>9;>12;>14;>15;>16;>18;>19l\E[1N$<10>
enter_appl_keypad_mode=\E[>10l
exit_appl_keypad_mode=\E[>10h
parm_mode_val-00000000-APPKEYON=>10
#
#     !!! IMPORTANT NOTE !!!
#     Set Terminal to: 8-bit-no-parity
#
use=ptclearcom.fc
use=ptcommon.fc
## >21 E2 mode  set=\E[>21h  reset=\E[>21l (default)
mode-00000000-00000100=\E[>21l
mode-00000100-00000000=\E[>21h
parm_mode_val-00000100-00000000=>21
use=pt2.fm
## ###############################################
### pt250 enter_ca_mode_2=\E[1Q\E[1;24y\E[12;>7;>10;>13h\E[4;>1;>3;>8;>9;>12;>14;>15;>16;>18;>19l\E[1N$<10>
### pt250-merge enter_ca_mode=\E[1N$<10/>\E[1Q\E[1;25y\E[12;>7;>10;>13;>25h\E[4;>1;>3;>8;>9;>12;>14;>15;>16;>18;>19l\E[4N$<10>
### rows_change-number-N=
rows_change-24-=\E[>25l
rows_change-25-N=\E[>25h
### rows_change_se_on_switch_only
rows_change_se_on_switch_only
### rows_change_does_clear_screen
rows_change_does_clear_screen
### rows_change_turns_auto_wrap_off
### rows_change_turns_auto_wrap_on
### rows_change_switch_page_number_0
### rows_change_clears_pages
### rows_change_sets_cursor_on
### rows_change_resets_insert_mode
rows_change_resets_insert_mode
### rows_change_resets_appl_keypad_mode
### rows_change_resets_keypad_xmit
### rows_change_resets_scroll_region
rows_change_resets_scroll_region
### rows_change_resets_character_set
### rows_change_resets_character_set
### rows_change_attributes_off--P=
### rows_change_resets_save_cursor
rows_change_resets_save_cursor
### rows_change_resets_origin_mode
### rows_change_sets_mode-00000000-00000000=
