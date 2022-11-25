# FacetTerm  qvt119  (80 column only)  description file 06/20/88
#    Uses 4 screen pages.
#    See also Qvt119 for 80/132 columns.
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: qvt119.FS,v 70.1 92/03/09 16:02:42 ssa Exp $
xon_xoff
columns=80
### NOT_IN_80 columns_wide=132
### NOT_IN_80 columns_wide_on=\En5
### NOT_IN_80 SCROLL columns_wide_on=\En6
### NOT_IN_80 columns_wide_off=\En4
pass=\En4
### NOT_IN_80 NO parm_mode_val-COLSWIDE-00000000=?3
### NOT_IN_80 NO columns_wide_clears_screen
lines=24
##--- MULTIPAGE ---##
pages=4
display_page_number=\Ew%p1%{1}%+%dP
##--- INIT ---##
### full_screen_page_mode 24_lines_page auto_wrap scroll
enter_ca_mode=\Ews\E\\1\En@\E'
use=qvt119com.fc
use=qvt.fm
