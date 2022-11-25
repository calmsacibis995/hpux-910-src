# FacetTerm  qvtpct  (80 column only)  description file 12/06/90
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: qvtpct.FS,v 70.1 92/03/09 16:49:31 ssa Exp $
xon_xoff
columns=80
pass-columns_wide_off=\E`:
lines=25
use=qpkeys.fc
##--- PRINTER ---##
transparent_print_on=^R
transparent_print_off=^T
##--- MULTIPAGE ---##
### pages=number
pages=4
### display_page_number=
display_page_number=\Ew%p1%{1}%+%dP
### parm_display_page_next=
### parm_display_page_prev=
### display_page_next=
### display_page_prev=
##--- INIT ---##
### full_screen_page_mode 24_lines_page auto_wrap scroll
enter_ca_mode=\Ews
use=qvtpct.fc
use=qvtpct.fm
