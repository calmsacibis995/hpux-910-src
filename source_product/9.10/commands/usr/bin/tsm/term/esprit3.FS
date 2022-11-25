# FacetTerm esprit3 (80 column only)  description file 08/20/91
#  Esprit III Color VDT - Tested on PCVision Emulator
# See also - Esprit3 for 80/132 column mode
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: esprit3.FS,v 70.1 92/03/09 16:54:37 ssa Exp $
xon_xoff
### graphics_8_bit
graphics_8_bit
### graphics_escape_control
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
enter_ca_mode=^O
use=esprit3com.fc
use=tvi900keys.fc
use=esprit3.fm
