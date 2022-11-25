# FacetTerm  qvt119wy50 (80 columns only)  description file 06/12/91
#    QVT 119+ in WYSE 50 mode with 4 screen pages.
#    See also Qvt119wy50 for 80/132 columns with 2 screen pages.
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: qvt119wy50.FS,v 70.1 92/03/09 16:02:54 ssa Exp $
xon_xoff
columns=80
### NOT_IN_80 columns_wide_off=\E`:
pass=\E`:
lines=24
use=wy50com.fc
use=wysekeys.fc
use=wy50.fm
use=wy50prnt.fc
##--- MULTI-PAGE ---##
pages=4
display_page_number=\Ew%p1%{1}%+%dP
##--- INIT ---##
enter_ca_mode=\EwP
