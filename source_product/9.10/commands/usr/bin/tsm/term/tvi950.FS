# FacetTerm  tvi950  (80 column only)  description file 07/22/91
#   TVI 950 - see also tvi950-4 for 4 screen pages
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: tvi950.FS,v 70.1 92/03/09 16:05:06 ssa Exp $
xon_xoff
### ?? graphics_8_bit
graphics_escape_control
columns=80
### NOT_IN_80 columns_wide=number
### NOT_IN_80 columns_wide_on=
### NOT_IN_80 columns_wide_off=
### NOT_IN_80 columns_wide_clears_screen
lines=24
##--- MULTIPAGE ---##
### pages=4
### ?? display_page_number=
### ?? parm_display_page_next=
### ?? parm_display_page_prev=
### display_page_next=\EK
### display_page_prev=\EJ
##--- INIT ---##
### xon_xoff auto_page_off 24_line_page
enter_ca_mode=^O\Ew\E\\1
use=tvi950com.fc
use=tvi900keys.fc
use=tvi950.fm
