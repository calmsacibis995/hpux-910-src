# FacetTerm  hp743wy50  (80/132 column)  description file 05/18/90
#    HP 700/43 in Wyse 50 mode with 4 screen pages.
#    See also hp743wy50 for 80 column only.
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: Hp743wy50.FS,v 70.1 92/03/09 15:53:26 ssa Exp $
xon_xoff
columns=80
lines=24
use=wy50com.fc
use=wysekeys.fc
use=wy50wid.fc
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
