# FacetTerm  3151-2425  (80 column only) description file 09/06/91
#   IBM 3151-31,41 in 24 line mode
#   switchable to 25 lines.
## Copyright (c) Structured Software Solutions, Inc. 1991.  All Rights Reserved.
## @(#) $Header: 3151-2425.FS,v 70.1 92/03/09 16:53:40 ssa Exp $
lines=24
columns=80
enter_ca_mode-24x80=\E r!! 8"P
rows_change-24=\E r!! 8"P
rows_change-25=\E r!! 9"P
### rows_change_se_on_switch_only
### rows_change_does_clear_screen
rows_change_does_clear_screen
### rows_change_resets_insert_mode
### rows_change_resets_scroll_region
rows_change_resets_scroll_region
### rows_change_resets_character_set
### rows_change_attributes_off--URBDOIPA=
rows_change_attributes_off--abcdeIOBUR
### rows_change_resets_save_cursor
### rows_change_clears_perwindow=d
use=31xxcom.fc
use=3151attr.fc
use=3151.fm
use=3151keys.fc
use=3151split.fc
use=3151stat.fc
use=3151ctrl.fc
##-- END --##
