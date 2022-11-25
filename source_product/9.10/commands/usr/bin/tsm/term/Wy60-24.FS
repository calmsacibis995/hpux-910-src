# FacetTerm  Wy60-24  (80/132 column)  description file  06/26/91
#     WYSE 60 in 24 line native mode using 2 screen pages.
## Copyright (c) Structured Software Solutions, Inc. 1991.  All Rights Reserved.
## @(#) $Header: Wy60-24.FS,v 70.1 92/03/09 16:48:07 ssa Exp $
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
pages=2
## 24_line=\Ee( econ80_off=\EeF
enter_ca_mode=\Ee(\EeF
ignore-forced-24_line=\Ee(
not_imp-25_line=\Ee)
not_imp-42_line=\Ee*
not_imp-43_line=\Ee+
#########################
use=wy60com.fc
use=wy60keys.fc
use=wy60edit.fc
use=wy60stat.fc
