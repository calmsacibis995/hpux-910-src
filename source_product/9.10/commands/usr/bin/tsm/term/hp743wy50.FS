# FacetTerm  hp743wy50  (80 column only)  description file 05/18/90
#    HP 700/43 in Wyse 50 mode with 4 screen pages.
#    See also Hp743wy50 for 80/132 column.
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: hp743wy50.FS,v 70.1 92/03/09 16:01:23 ssa Exp $
xon_xoff
columns=80
pass-columns_wide_off=\E`:
lines=24
use=wy50ans.fc
use=wy50com.fc
use=wysekeys.fc
use=wy50.fm
use=wy50prnt.fc
##--- MULTI-PAGE ---##
pages=4
display_page_number=\Ew%p1%d
### USE_DISPLAY_PAGE_NUMBER display_page_next=\EwC
### USE_DISPLAY_PAGE_NUMBER display_page_prev=\EwB
##--- INIT ---##
## wyse_50_mode=\E~" 24_line=\Ee( enable_4_24_line_pages=\EwG caps_lock_off=\Ee'
## enhanced_mode=\E~!
enter_ca_mode=\E~"\E~!\EeG\Ee(\EwG\Ee'
