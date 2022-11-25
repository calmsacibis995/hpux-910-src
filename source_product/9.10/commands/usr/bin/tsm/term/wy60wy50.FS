# FacetTerm  wy60wy50 (80 columns only)  description file 05/18/90
#    WYSE 60 in WYSE 50 mode with 6 screen pages.
#    See also Wy60wy50 for 80/132 columns with 4 screen pages.
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: wy60wy50.FS,v 70.1 92/03/09 16:10:09 ssa Exp $
xon_xoff
columns=80
### NOT_IN_80 columns_wide_off=\E`:
pass=\E`:
lines=24
use=wy50com.fc
use=wysekeys.fc
use=wy50.fm
##--- MULTI-PAGE ---##
pages=6
display_page_number=\Ew%p1%d
### USE_DISPLAY_PAGE_NUMBER display_page_next=\EwC
### USE_DISPLAY_PAGE_NUMBER display_page_prev=\EwB
##--- INIT ---##
## econ_80_on=\EeG 24_line=\Ee( page=lines=\EwG caps_lock_off=\Ee'
enter_ca_mode=\EeG\Ee(\EwG\Ee'
##--- PRINT ---##
not_imp-copy_print_on=^R
transparent_print_on=\Ed#
transparent_print_off=^T
##--- MODE ---##
pass-time_of_day=\Ec8%4f1
## caps_lock_off=\Ee' caps_lock_on=\Ee&
mode-00000000-00000100=\Ee'
mode-00000100-00000000=\Ee&
##### NOT_USED has_meta_key
