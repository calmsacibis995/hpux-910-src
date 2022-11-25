# FacetTerm  tvi955-2  (80 column only ) description file 06/28/88
#   TVI 955 in embedded attribute mode with 2 screen pages.
#   See also  tvi955 for no screen pages.
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: tvi955-2.FS,v 70.1 92/03/09 16:05:17 ssa Exp $
xon_xoff
### ?? graphics_8_bit
graphics_escape_control
columns=80
### NOT_IN_80 columns_wide=132
### NOT_IN_80 columns_wide_on=\E[=3h
columns_wide_off=\E[=3l
### NOT_IN_80 columns_wide_clears_screen
lines=24
##--- MULTIPAGE ---##
pages=2
display_page_number=\E[1;%p1%d}
### ?? parm_display_page_next=
### ?? parm_display_page_prev=
### USE_NUMBER display_page_next=\EK
### USE_NUMBER display_page_prev=\EJ
##--- INIT ---##
### xon_xoff auto_page_off 24_line_page
enter_ca_mode=^O\Ew\E\\1
use=tvi955com.fc
use=tvi955keys.fc
use=tvi.fm
