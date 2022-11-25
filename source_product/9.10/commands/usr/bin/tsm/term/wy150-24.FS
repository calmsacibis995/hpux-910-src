# FacetTerm  wy150-24  (80 column only)  description file  07/29/91
#     WYSE 150 in 24 line native mode using 3 screen pages.
## Copyright (c) Structured Software Solutions, Inc. 1991.  All Rights Reserved.
## @(#) $Header: wy150-24.FS,v 70.1 92/03/09 16:53:20 ssa Exp $
use=wy60ans.fc
use=wy60.fm
#########################
## wy60=\E~4 xon-xoff=\Ec21 ack_mode_off(not wy150)=\Ee6
enter_ca_mode=\E~4\Ec21\Ee6
ignore-forced-wy60=\E~4
pass-xon_xoff=\Ec21
pass-ack_mode_off=\Ee6
### replacement for ack_mode_off on wy60 ???
enter_ca_mode-enhance_mode_off=\E~\040
#########################
use=wy60narr.fc
lines=24
pages=3
## 24_line=\Ee( econ80=\EeG
enter_ca_mode-24_line_econ80=\Ee(\EeG
ignore-forced-24_line=\Ee(
not_imp-25_line=\Ee)
not_imp-42_line=\Ee*
not_imp-43_line=\Ee+
#########################
use=wy60com.fc
use=wy150keys.fc
use=wy60edit.fc
use=wy60stat.fc
