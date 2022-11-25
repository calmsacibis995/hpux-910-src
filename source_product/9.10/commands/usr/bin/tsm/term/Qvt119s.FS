# FacetTerm  Qvt119s  (80/132 columns)  description file 06/20/88
#    SCROLL MODE
#    see also Qvt119 for 80/132 column with all 132 on screen.
#    see also qvt119 for 80 column only with 4 screen pages.
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: Qvt119s.FS,v 70.1 92/03/09 15:53:37 ssa Exp $
xon_xoff
columns=80
columns_wide=132
### NOT_THIS_MODE columns_wide_on=\En5
columns_wide_on=\En6
columns_wide_off=\En4
### NO columns_wide_clears_screen
### NOT_IN_COMMON NO parm_mode_val-COLSWIDE-00000000=?3
lines=24
##--- INIT ---##
### full_screen_page_mode 24_lines_page auto_wrap scroll
enter_ca_mode=\Ews\E\\1\En@\E'
use=qvt119com.fc
use=qvt.fm
