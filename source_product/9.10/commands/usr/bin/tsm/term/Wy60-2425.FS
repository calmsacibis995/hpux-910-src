# FacetTerm  Wy60-2425  (80/132 column)  description file  06/26/91
#     WYSE 60 in 24 line native mode using 1 screen pages.
#     switchable to 25 lines.
## Copyright (c) Structured Software Solutions, Inc. 1991.  All Rights Reserved.
## @(#) $Header: Wy60-2425.FS,v 70.1 92/03/09 16:51:04 ssa Exp $
use=wy60ans.fc
use=wy60.fm
#########################
## wy60=\E~4 xon-xoff=\Ec21 ack_mode_off=\Ee6
enter_ca_mode=\E~4\Ec21\Ee6
ignore-forced-wy60=\E~4
pass-xon_xoff=\Ec21
pass-ack_mode_off=\Ee6
#########################
use=wy60wide.fc
lines=24
pages=1
## 24_line=\Ee( econ80_off=\EeF
enter_ca_mode=\Ee(\EeF
rows_change-24=\Ee(
rows_change-25=\Ee)
rows_change_switch_page_number_0
rows_change_does_clear_screen
rows_change_clears_pages
rows_change_attributes_off--URBDI
rows_change_resets_scroll_region
#########################
use=wy60com.fc
use=wy60keys.fc
use=wy60edit.fc
use=wy60stat.fc
