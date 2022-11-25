# FacetTerm  wy60addsvp  (80 column only)  description file 04/23/90
#    WYSE 60 in ADDS Viewpoint mode 6 screen pages.
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: wy60addsvp.FS,v 70.1 92/03/09 16:11:28 ssa Exp $
xon_xoff
columns=80
### NOT_IN_80 columns_wide=132
### NOT_IN_80 columns_wide_on=\E`;
### NOT_IN_80 columns_wide_off=\E`:
pass=\E`:
### NO columns_wide_clears_screen
lines=24
use=addsvp.fc
##--- MULTI-PAGE ---##
pages=6
display_page_number=\Ew%p1%d
### USE_DISPLAY_PAGE_NUMBER display_page_next=\EwC
### USE_DISPLAY_PAGE_NUMBER display_page_prev=\EwB
##--- INIT ---##
## econ_80_on=\EeG 24_line=\Ee( page=lines=\EwG caps_lock_off=\Ee'
enter_ca_mode=\EeG\Ee(\EwG\Ee'
use=addsvp.fm
