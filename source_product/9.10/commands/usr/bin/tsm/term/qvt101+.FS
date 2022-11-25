# FacetTerm  qvt101+  (80 column only)  description file 11/12/90
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: qvt101+.FS,v 70.1 92/03/09 16:49:25 ssa Exp $
xon_xoff
columns=80
pass-columns_wide_off=\E`:
lines=24
use=q101+keys.fc
##--- PRINTER ---##
transparent_print_on=^R
transparent_print_off=^T
##--- MULTIPAGE ---##
### pages=number
pages=4
### display_page_number=
display_page_number=\E-%p1%{1}%+%d11
### parm_display_page_next=
### parm_display_page_prev=
### display_page_next=
### display_page_prev=
##--- INIT ---##
enter_ca_mode=\E-1\040\040
use=qvt101+.fc
use=qvt.fm
